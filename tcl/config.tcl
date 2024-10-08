###
### config.tcl: Some embedded configuration for Scid

namespace eval ::config {}

if {$windowsOS} {
  set scidShareDir $scidExeDir
} elseif {$macOS} {
  set scidShareDir [file normalize [file join $scidExeDir "../Resources"]]
} else {
  if {![info exists scidShareDir]} {
    set scidShareDir [file normalize [file join $scidExeDir "../share/scid"]]
  }
}

# also reset BooksDir if the variable is set, but doesn't exist S.A.
if {![info exists scidBooksDir] || ![file isdirectory $scidBooksDir]} {
  set scidBooksDir [file nativename [file join $scidShareDir "books"]]
}

if {![info exists scidBasesDir] || ![file isdirectory $scidBasesDir]} {
  set scidBasesDir [file nativename [file join $scidShareDir "bases"]]
}

# ecoFile: the ECO file for opening classification. Scid will try to load
# this first, and if that fails, it will try to load  "scid.eco" in the
# current directory.
if {$ecoFile == "" || ![file exists $ecoFile]} {
  if {$windowsOS} {
    set ecoFile [file join $scidDataDir "scid.eco"]
  } else {
    set ecoFile [file join [file join $scidShareDir "data"] "scid.eco"]
  }
}

# Spell-checking file: default is "spelling.ssp".
if {![info exists spellCheckFile] || ![file exists $spellCheckFile]} {
  if {$windowsOS} {
    set spellCheckFile [file join $scidDataDir "spelling.ssp"]
  } else {
    set spellCheckFile [file join $scidShareDir "spelling.ssp"]
  }
}

### Display directories

::splash::add "scidShareDir is $scidShareDir"

if {[file isdirectory $::scidBasesDir]} {
  ::splash::add "scidBasesDir is $scidBasesDir"
} else {
  ::splash::add "scidBasesDir $scidBasesDir not found!" error
}

if {[file isdirectory $::scidBooksDir]} {
  ::splash::add "scidBooksDir is $scidBooksDir"
} else {
  ::splash::add "scidBooksDir $scidBooksDir not found!" error
}

### Setup for truetype (and PGN figurine) support

array set graphFigurineFamily {}
array set graphFigurineWeight { normal normal bold normal }
set graphFigurineAvailable [expr $windowsOS || $macOS]
if {[::tk windowingsystem] eq "x11"} {
  catch { if {[::tk::pkgconfig get fontsystem] eq "xft"} { set graphFigurineAvailable 1 } }
}

if {$graphFigurineAvailable} {
  set graphFigurineFamilies {}
  foreach font [font families] {
    if {[string match -nocase {Scid Chess *} $font]} { lappend graphFigurineFamilies $font }
  }
  if {[lsearch $graphFigurineFamilies {Scid Chess Traveller}] >= 0} {
    set graphFigurineFamily(normal) {Scid Chess Traveller}
  } elseif {[llength $graphFigurineFamilies] > 0} {
    set graphFigurineFamily(normal) [lindex $graphFigurineFamilies 0]
  } else {
    set graphFigurineAvailable 0
    set useGraphFigurine 0
  }
  if {$graphFigurineAvailable} {
    if {[lsearch $graphFigurineFamilies {Scid Chess Standard}] >= 0} {
      set graphFigurineFamily(bold) {Scid Chess Standard}
      set graphFigurineWeight(bold) bold
    } else {
      set graphFigurineFamily(bold) $graphFigurineFamily(normal)
    }
  }
} else {
  set useGraphFigurine 0
}

if {$graphFigurineAvailable} {
  ::splash::add "PGN chess figures (True type fonts) enabled."
} else {
  ::splash::add "PGN chess figures (True type fonts) disabled."
}

if {$graphFigurineAvailable} {
  font create font_Figurine(normal) -family $graphFigurineFamily(normal) -weight $graphFigurineWeight(normal) -size [lindex $fontOptions(Regular) 1]
  font create font_Figurine(bold) -family $graphFigurineFamily(bold) -weight $graphFigurineWeight(bold) -size [lindex $fontOptions(Regular) 1]
}

if {$unixOS && !$macOS} {
  if {![catch {package require BWidget}]} {
    ::splash::add "BWidget found! Enabling custom Colour Selector"
    set i 0
    foreach c $::bwidgetBackgrounds {
      ::SelectColor::setcolor $i $c
      incr i
    }
    proc tk_chooseColor {args} {
      if {[set i [lsearch -exact $args -initialcolor]] > -1} {
        set args [lreplace $args $i $i -color]
      }
      set result [eval ::SelectColor::dialog .bwidget $args]
      set ::bwidgetBackgrounds $::SelectColor::_userColors
      return $result
    }
  }
}

### end of config.tcl

