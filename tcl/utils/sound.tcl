### sound.tcl
# Scid vs. PC sound procedures
# Copyright (C) Shane Hudson 2004, Stevenaaus 2013, 2026
# Requires the Tcl/Tk sound package Snack , Pulse Audio or similar

namespace eval ::utils::sound {}

set ::utils::sound::hasSnackPackage 0
set ::utils::sound::hasCmdPlayer 0
set ::utils::sound::cmdPipe ""
set ::utils::sound::cmdPid ""
set ::utils::sound::isPlayingSound 0
set ::utils::sound::soundQueue {}
set ::utils::sound::soundFiles "King Queen Rook Bishop Knight CastleQ CastleK Back Mate Promote Check a b c d e f g h x 1 2 3 4 5 6 7 8 move start end"

#   Maps characters in a move to sounds.
#   Before this map is used, "O-O-O" is converted to "q" and "O-O" to "k"
#   Also note that "U" (undo) is used for taking back a move.

array set ::utils::sound::soundMap {
  K King Q Queen R Rook B Bishop N Knight k CastleK q CastleQ
  x x U Back # Mate = Promote  + Check
  a a b b c c d d e e f f g g h h
  1 1 2 2 3 3 4 4 5 5 6 6 7 7 8 8
}

proc ::utils::sound::Setup {} {
  variable hasSnackPackage
  variable hasCmdPlayer
  variable soundFiles
  variable soundFolder
  variable soundCommand

  ::splash::add "Initializing Sound..."

  # Expand "~" in a user-set folder so file joins below work
  if {$soundFolder ne ""} {
    set soundFolder [file normalize $soundFolder]
  }

  if {![catch {package require snack 2.0}]} {
    set hasSnackPackage 1
    ::splash::add "   Snack sound package found."
    set devices [snack::audio outputDevices]
    ::splash::add "   Available output devices are: $devices"

    if {$::utils::sound::device != {}} {
      ::utils::sound::SetDevice
    } else {
      set ::utils::sound::device [lindex $devices 0]
    }

    # Set up sounds. Each sound will be empty until a WAV file for it is found.
    foreach soundFile $soundFiles {
      ::snack::sound sound_$soundFile
    }
  } else {
    ::splash::add "   Snack sound package not found."
    set hasSnackPackage 0
  }

  # Alternative backend: play the sound files via an external command
  # (Default is PulseAudio "paplay")
  set hasCmdPlayer [expr {[auto_execok $soundCommand]  != ""}]

  if {!$hasSnackPackage} {
    ::splash::add "   Snack sound package not found."
    # proc ::utils::sound::AnnounceMove {move} {}
    # proc ::utils::sound::PlayMove {sound} {}
    # return
  }
  if {$hasCmdPlayer} {
    ::splash::add "   Sound player $soundCommand found."
  }

  set numSounds [::utils::sound::ReadFolder]
}

proc ::utils::sound::AnnounceNewMove {move} {
  if {$::utils::sound::useCmd && $::utils::sound::announceNew || ($::fics::sound && ($::fics::playing == 1 || $::fics::playing == -1))} { AnnounceMove $move }
}

proc ::utils::sound::AnnounceForward {move} {
  if {$::utils::sound::useCmd && $::utils::sound::announceForward} { AnnounceMove $move }
}


proc ::utils::sound::AnnounceBack {} {
  if {$::utils::sound::useCmd && $::utils::sound::announceBack} { AnnounceMove U }
}

proc ::utils::sound::AnnounceMove {move} {
  variable soundMap
  variable announceTock

  if {$::utils::sound::useCmd == 0} {return}

  if {$announceTock || ($::fics::sound && ($move == "tock" || $::fics::playing == 1 || $::fics::playing == -1))} {
    PlayMove sound_move
    return
  }

  if {[string range $move 0 4] == "O-O-O"} { set move q }
  if {[string range $move 0 2] == "O-O"} { set move k }
  set move [::untrans $move]
  set parts [split $move ""]
  set soundList {}
  foreach part $parts {
    if {[info exists soundMap($part)]} {
      lappend soundList sound_$soundMap($part)
    }
  }
  if {[llength $soundList] > 0} {
    PlayMove $soundList
  }
}

# Plays a sound or list of sounds representing a move

proc ::utils::sound::PlayMove {soundlist} {
  if {$::utils::sound::useCmd == 0} {return}
  if {$::utils::sound::useCmd == 1} {::snack::audio stop}
  if {$::utils::sound::useCmd == 2} {::utils::sound::StopCmd}
  set ::utils::sound::isPlayingSound 0
  set ::utils::sound::soundQueue $soundlist
  after cancel ::utils::sound::PlaySoundQueue
  after idle ::utils::sound::PlaySoundQueue
}

# External-command backend: sounds of one move are chained through
# fileevent ( each player process starts the next on completion ), so a
# whole phrase like "Queen d4 check" plays in order. A new move stops
# the running chain ( like snack::audio stop ), so rapid play never
# blocks and never overlaps across moves.

proc ::utils::sound::PlayCmd {player files} {
  variable cmdPipe
  variable cmdFiles
  variable cmdPlayer
  ::utils::sound::StopCmd
  set cmdFiles $files
  set cmdPlayer $player
  ::utils::sound::StartCmdSound
}

proc ::utils::sound::StartCmdSound {} {
  variable cmdPipe
  variable cmdPid
  variable cmdFiles
  variable cmdPlayer
  if {[llength $cmdFiles] == 0} {
    set ::utils::sound::isPlayingSound 0
    return
  }
  set f [lindex $cmdFiles 0]
  set cmdFiles [lrange $cmdFiles 1 end]
  if {[catch {open "|$cmdPlayer [list $f]" r} pipe]} {
    # Player failed for this file: continue with the next one.
    ::utils::sound::StartCmdSound
    return
  }
  set cmdPipe $pipe
  # Remember the player PID so a new move can really truncate
  # the running file (close alone would let paplay play on).
  catch {set cmdPid [pid $cmdPipe]}
  fconfigure $cmdPipe -blocking 0
  fileevent $cmdPipe readable [list ::utils::sound::CmdDone]
}

proc ::utils::sound::CmdDone {} {
  variable cmdPipe
  variable cmdPid
  if {![info exists cmdPipe] || $cmdPipe == ""} { return }
  set pipe $cmdPipe
  set cmdPipe ""
  set cmdPid ""
  catch {close $pipe}
  ::utils::sound::StartCmdSound
}

proc ::utils::sound::StopCmd {} {
  variable cmdPipe
  variable cmdPid
  variable cmdFiles
  if {[info exists cmdPipe] && $cmdPipe != ""} {
    # Truncate like snack::audio stop: kill the running player first,
    # close alone would let the current file play to the end.
    catch {fileevent $cmdPipe readable {}}
    if {[info exists cmdPid] && $cmdPid != ""} {
      catch {exec -- kill $cmdPid}
    }
    catch {close $cmdPipe}
    set cmdPipe ""
    set cmdPid ""
  }
  set cmdFiles {}
}

proc ::utils::sound::PlaySoundQueue {} {
  if {$::utils::sound::useCmd == 2} {
    # External-command backend
    # Collect this move's wav files and hand the whole list to the chaining player (see PlayCmd above)
    set player [auto_execok $::utils::sound::soundCommand]
    if {$player == ""} { return }
    set files {}
    foreach sound $::utils::sound::soundQueue {
	set fname [string range $sound 6 end]
	set f [file join $::utils::sound::soundFolder $fname.wav]
	if {![file readable $f]} { continue }
	lappend files $f
    }
    if {[llength $files] > 0} {
      incr ::utils::sound::isPlayingSound
      ::utils::sound::PlayCmd $player $files
    }
  } else {
    # Snack
    set ::utils::sound::isPlayingSound 0

    while {[llength $::utils::sound::soundQueue] > 0} {
      set sound [lindex $::utils::sound::soundQueue 0]
      set ::utils::sound::soundQueue [lrange $::utils::sound::soundQueue 1 end]
      incr ::utils::sound::isPlayingSound
      $sound play -blocking 0 -command "incr ::utils::sound::isPlayingSound -1"
      if {[llength $::utils::sound::soundQueue] > 0} {
	vwait ::utils::sound::isPlayingSound
      }
    }
  }
}


proc ::utils::sound::OptionsDialog {} {
  variable hasCmdPlayer

  set w .soundOptions

  if {[winfo exists $w]} {
    raiseWin $w
    return
  }

  # foreach v {soundFolder announceNew announceForward announceBack announceTock}

  toplevel $w
  wm title $w "Scid: Sound Options"
  wm withdraw $w
  wm resizable $w 0 0

  pack [frame $w.b] -side bottom -fill x -pady 2

  if {! $::utils::sound::hasSnackPackage} {
    label $w.status -text "Scid could not find the Snack audio package at startup. Sound is disabled."
    pack $w.status -side bottom
  }

  pack [frame $w.f -relief groove -borderwidth 2] \
      -side top -fill x -padx 5 -pady 5 -ipadx 4 -ipady 4

  set f $w.f
  set r 0

  ### Sound system: snack or an external command ( PulseAudio paplay )

  label $f.sysLabel -text "Sound System" -font font_Bold
  grid $f.sysLabel -row $r -column 0 -columnspan 3 -pady 4
  incr r

  radiobutton $f.noSound  -text None    -variable ::utils::sound::useCmd -value 0 -command ::utils::sound::checkUseCmd
  radiobutton $f.useSnack -text Snack   -variable ::utils::sound::useCmd -value 1 -command ::utils::sound::checkUseCmd
  radiobutton $f.useCmd   -text Command -variable ::utils::sound::useCmd -value 2 -command ::utils::sound::checkUseCmd
  grid $f.useSnack -row $r -column 0 -sticky w -pady 4
  grid $f.useCmd   -row $r -column 1 -sticky w -pady 4
  grid $f.noSound  -row $r -column 2 -sticky w -pady 4
  incr r

  ### snack device combobox

  label $f.deviceLabel -text "Snack Sound Device" 
  grid  $f.deviceLabel -row $r -column 0  -sticky w -pady 2
  set tmp $::utils::sound::device
  if {$::utils::sound::hasSnackPackage} {
    set devices [::snack::audio outputDevices]
  } else {
    $f.useSnack configure -state disabled
    set tmp 0
    set devices {}
  }
  ttk::combobox $f.device -width 20 -textvariable ::utils::sound::device
  bind $f.device <<ComboboxSelected>> ::utils::sound::SetDevice

  $f.device configure -values $devices
  set ::utils::sound::device $tmp

  grid $f.device -row $r -column 1 -pady 2
  incr r

  ## soundCommand
  label $f.cmdLabel -text "Sound Command"
  entry $f.cmdEntry -width 12 -textvariable ::utils::sound::soundCommand -state [expr {$::utils::sound::useCmd ? "normal" : "disabled"}]
  grid $f.cmdLabel -row $r -column 0 -sticky w -pady 2
  grid $f.cmdEntry -row $r -column 1 -sticky w -pady 2
  incr r

  ::utils::sound::checkUseCmd

  label $f.ftitle -text $::tr(SoundsFolder) -font font_Bold
  grid $f.ftitle -row $r -column 0 -columnspan 3 -pady 4
  incr r

  entry $f.folderEntry -width 40 -textvariable ::utils::sound::soundFolder
  grid $f.folderEntry -row $r -column 0 -columnspan 2 -sticky we -padx 3
  button $f.folderBrowse -text " $::tr(Browse)" -command ::utils::sound::ChooseFolder
  grid $f.folderBrowse -row $r -column 2 -padx 3
  incr r

  grid [frame $f.gap$r -height 5] -row $r -column -0; incr r

  label $f.title -text $::tr(SoundsAnnounceOptions) -font font_Bold
  grid $f.title -row $r -column 0 -columnspan 3 -pady 4
  incr r

  checkbutton $f.announceNew -text $::tr(SoundsAnnounceNew) \
      -variable ::utils::sound::announceNew
  grid $f.announceNew -row $r -column 0 -columnspan 2 -sticky w
  incr r

  grid [frame $f.gap$r -height 5] -row $r -column -0; incr r

  checkbutton $f.announceForward -text $::tr(SoundsAnnounceForward) \
      -variable ::utils::sound::announceForward
  grid $f.announceForward -row $r -column 0 -columnspan 2 -sticky w
  incr r

  grid [frame $f.gap$r -height 5] -row $r -column -0; incr r

  checkbutton $f.announceBack -text $::tr(SoundsAnnounceBack) \
      -variable ::utils::sound::announceBack
  grid $f.announceBack -row $r -column 0 -columnspan 2 -sticky w
  incr r

  grid [frame $f.gap$r -height 5] -row $r -column -0; incr r

  checkbutton $f.announceTock -text {Play Tick-Tock sound instead of move} \
      -variable ::utils::sound::announceTock
  grid $f.announceTock -row $r -column 0 -columnspan 2 -sticky w
  incr r

  grid [frame $f.gap$r -height 5] -row $r -column -0; incr r
  dialogbutton $w.b.help -text $::tr(Help) -command {helpWindow Sound}
  dialogbutton $w.b.ok -text OK -command " ::utils::sound::Setup ; destroy $w"
  dialogbutton $w.b.apply -textvar ::tr(Apply) -command ::utils::sound::Setup
  packbuttons right $w.b.ok $w.b.apply $w.b.help
  bind $w <Escape> "destroy $w"
  bind $w <F1> {helpWindow Sound}
  placeWinCenter $w
  wm state $w normal
}

proc ::utils::sound::checkUseCmd {} {
  global ::utils::sound::useCmd

  set f .soundOptions.f

  $f.cmdEntry    configure -state disabled
  $f.cmdLabel    configure -state disabled
  $f.deviceLabel configure -state disabled
  $f.device      configure -state disabled
  if {$::utils::sound::useCmd == 1} {
    $f.deviceLabel configure -state normal
    $f.device      configure -state normal
    return
  }
  if {$::utils::sound::useCmd == 2} {
    $f.cmdEntry    configure -state normal
    $f.cmdLabel    configure -state normal
  }
}

proc ::utils::sound::ChooseFolder {} {
  if {[file isdirectory $::utils::sound::soundFolder]} {
    set initialdir $::utils::sound::soundFolder
  } else {
    set initialdir $::env(HOME)
  }
  set newFolder [tk_chooseDirectory -initialdir $initialdir -parent .soundOptions -title "Scid: $::tr(SoundsFolder)"]
  if {$newFolder == ""} {
    return
  }
  set ::utils::sound::soundFolder [file nativename $newFolder]
  ::utils::sound::soundFolderOK
}

proc ::utils::sound::soundFolderOK {} {
  variable soundFolder

    set numSoundFiles [::utils::sound::ReadFolder]
    tk_messageBox -title "Scid: Sound Files" -type ok -icon info \
        -message "Found $numSoundFiles of [llength $::utils::sound::soundFiles] sound files in $::utils::sound::soundFolder"
}

### Read sound files from the specified directory.
#   Return the number of Scid sound files found in that directory.

proc ::utils::sound::ReadFolder {} {
  variable soundFiles
  variable soundFolder

  set count 0
  set missing {}
  set numSought [llength $soundFiles]

  foreach soundFile $soundFiles {
    set f [file join $soundFolder $soundFile.wav]
    if {[file readable $f]} {
      if {$::utils::sound::hasSnackPackage} {
        sound_$soundFile configure -file $f

        ### alternatively, store file in memory
        # sound_$soundFile read $f
      }
      incr count
    } else {
      lappend missing $soundFile
    }
  }
  ::splash::add "   Found $count of $numSought sound files in $soundFolder"
  if {$count > 0} {
    ::splash::add "   Missing Sound files are: $missing"
  }
  return $count
}

proc ::utils::sound::SetDevice {args} {

  if {[catch {snack::audio selectOutput $::utils::sound::device}]} {
    ::splash::add "   Failure setting Snack output device to $::utils::sound::device"
  } else {
    ::splash::add "   Succesfully set Snack output device to $::utils::sound::device"
  }
}

::utils::sound::Setup

# end of sound.tcl
