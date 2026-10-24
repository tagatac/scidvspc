### history.tcl
### Text entry history functions for Scid.
### Copyright (C) 2004 Shane Hudson.

namespace eval ::utils::history {}


set ::utils::history::defaultListLength 10
array set ::utils::history::listLength {}
array set ::utils::history::comboboxWidget {}

if {! [info exists ::utils::history::listData]} {
  array set ::utils::history::listData {}
}

# Load any history lists that were saved in the last session:
catch {source [scidConfigFile history]}


proc ::utils::history::SetList {key list} {
  set ::utils::history::listData($key) $list
}


proc ::utils::history::GetList {key} {
  variable listData
  if {[info exists listData($key)]} {
    return $listData($key)
  }
  return {}
}


#  ::utils::history::AddEntry
#
#   Inserts an entry at the top of the list of the specified key and
#   ensures that only one copy of the entry exists in the list.
#   The list is then pruned to the limit size is necessary.
#
proc ::utils::history::AddEntry {key entry} {
  variable listData
  # We do not add the empty string to a history list:
  if {$entry == "" } {
    return
  }

  if {[info exists listData($key)]} {
    # Take out this entry if it exists, so it will not appear twice:
    set index [lsearch -exact $listData($key) $entry]
    if {$index == 0} {
      # The entry is already at the start of the list; nothing to do
      return
    } elseif {$index > 0} {
      set listData($key) [lreplace $listData($key) $index $index]
    }
    set listData($key) [linsert $listData($key) 0 $entry]
    ::utils::history::PruneList $key
  } else {
    set listData($key) [list $entry]
  }
  RefillCombobox $key

  if { [llength [GetList $key]] > 0 } {
    set cb [ GetCombobox $key ]
    if { $cb != "" && [winfo exists $cb]} {
      $cb current 0
      $cb icursor end
    }
  }

}


proc ::utils::history::SetLimit {key length} {
  set ::utils::history::listLength($key) $length
  ::utils::history::PruneList $key
}


proc ::utils::history::GetLimit {key} {
  variable listLength
  variable defaultListLength
  if {[info exists ::utils::history::listLength($key)]} {
    return $::utils::history::listLength($key)
  }
  return $defaultListLength
}


proc ::utils::history::PruneList {key {length -1}} {
  variable listData
  if {! [info exists listData($key)]} { return }
  if {$length < 0} {
    set length [::utils::history::GetLimit $key]
  }
  set listData($key) [lrange $listData($key) 0 [expr {$length - 1}]]
}



# ::utils::history::SetCombobox
#
#   Returns the combobox widget associated with this history key.
#
proc ::utils::history::GetCombobox {key} {
  variable comboboxWidget
  if {[info exists comboboxWidget($key)]} {
    return $comboboxWidget($key)
  }
  return ""
}


# ::utils::history::SetCombobox
#
#   Associates the specified widget (which must be a megawidget created
#   with ::combobox::combobox, see contrib/combobox.tcl) with the specifiec
#   history key. Whenever the list for this history key is modified, the
#   combobox widget will be updated.
#   If nameType (player, event, site or round) is given, matching names
#   of the current database are suggested while typing.
#
proc ::utils::history::SetCombobox {key cbWidget {nameType {}}} {
  set ::utils::history::comboboxWidget($key) $cbWidget
  RefillCombobox $key
  if {$nameType == {}} { return }
  set ::utils::history::suggest($cbWidget) $nameType
  bindtags $cbWidget [concat NameSuggest [bindtags $cbWidget]]
}

### Name suggestions: a list below the combobox which does not take the focus.
#   Up/Down select, Return takes the selected name, Escape closes the list.
#   The list is destroyed whenever it is not shown. Author P.G.

set ::utils::history::suggest(cb) {}
set ::utils::history::suggest(text) {}

bind NameSuggest <KeyRelease> {::utils::history::Suggest %W}
bind NameSuggest <Down> {if {[winfo exists .nameSuggest]} {::utils::history::SuggestMove 1 ; break}}
bind NameSuggest <Up> {if {[winfo exists .nameSuggest]} {::utils::history::SuggestMove -1 ; break}}
bind NameSuggest <Return> {if {[::utils::history::SuggestTake]} break}
bind NameSuggest <Escape> {if {[winfo exists .nameSuggest]} {destroy .nameSuggest ; break}}
bind NameSuggest <FocusOut> {destroy .nameSuggest}
bind NameSuggest <Destroy> {destroy .nameSuggest}

proc ::utils::history::Suggest {cb} {
  variable suggest
  set text [$cb get]
  if {$cb == $suggest(cb) && $text == $suggest(text)} { return }
  set suggest(cb) $cb
  set suggest(text) $text
  set nameType $suggest($cb)

  set found {}
  if {$text != {}} {
    # Also try with capitals at the start of each word, and umlauts as ae/oe/ue/ss or back
    set words {}
    foreach word [split $text " "] { lappend words [string toupper $word 0 0] }
    set prefixes {}
    foreach prefix [list [join $words " "] [string toupper $text 0 0] $text] {
      lappend prefixes $prefix \
          [string map {\u00e4 ae \u00f6 oe \u00fc ue \u00df ss \u00c4 Ae \u00d6 Oe \u00dc Ue} $prefix] \
          [string map {ae \u00e4 oe \u00f6 ue \u00fc Ae \u00c4 Oe \u00d6 Ue \u00dc} $prefix]
    }
    foreach prefix [lsort -unique $prefixes] {
      catch {
        foreach {count name elo} [sc_name match -freq -elo $nameType $prefix 100] {
          lappend found [list $count $name $elo]
        }
      }
    }
    # First name first: "Garry K" also finds "Kasparov, Garry"
    if {$nameType == "player" && [string first , $text] < 0 && [regexp {^(.+) (\S+)$} $text -> first last]} {
      catch {
        foreach {count name elo} [sc_name match -freq -elo player [string toupper $last 0 0] 10000] {
          if {[string match -nocase "*, $first*" $name]} { lappend found [list $count $name $elo] }
        }
      }
    }
  }

  # The most frequent names first
  set found [lrange [lsort -integer -decreasing -index 0 $found] 0 99]
  set w .nameSuggest
  if {$found == {}} {
    destroy $w
    return
  }
  if {! [winfo exists $w]} {
    toplevel $w
    wm overrideredirect $w 1
    listbox $w.list -width 0 -exportselection 0 -activestyle none -relief solid -borderwidth 1 \
        -yscrollcommand "$w.sb set"
    scrollbar $w.sb -command "$w.list yview"
    pack $w.list -side left -fill both -expand 1
    bind $w.list <Motion> {%W selection clear 0 end ; %W selection set @%x,%y}
    bind $w.list <1> {%W selection clear 0 end ; %W selection set @%x,%y ; break}
    bind $w.list <ButtonRelease-1> {::utils::history::SuggestTake ; break}
  }
  set font [$cb cget -font]
  if {$font == {}} { set font TkTextFont }
  $w.list configure -font $font -height [expr {min(20, [llength $found])}]
  if {[llength $found] > 20} {
    pack $w.sb -side right -fill y -before $w.list
  } else {
    pack forget $w.sb
  }
  $w.list delete 0 end
  foreach f $found {
    lassign $f count name elo
    $w.list insert end $name
    # Mark strong players
    if {$elo >= 2500} { $w.list itemconfigure end -foreground blue3 }
  }
  wm geometry $w +[winfo rootx $cb]+[expr {[winfo rooty $cb] + [winfo height $cb]}]
  wm minsize $w [winfo width $cb] 1
  raise $w
  update idletasks ;# MacOS only ??
}

proc ::utils::history::SuggestMove {dir} {
  set lb .nameSuggest.list
  set i [$lb curselection]
  if {$i == {}} {
    set i [expr {$dir > 0 ? 0 : [$lb size] - 1}]
  } else {
    set i [expr {$i + $dir}]
  }
  if {$i >= 0 && $i < [$lb size]} {
    $lb selection clear 0 end
    $lb selection set $i
    $lb see $i
  }
}

# Puts the selected name into the combobox. Returns 1 if a name was taken.
proc ::utils::history::SuggestTake {} {
  variable suggest
  set w .nameSuggest
  if {! [winfo exists $w]} { return 0 }
  set i [$w.list curselection]
  if {$i != {}} { set suggest(text) [$w.list get $i] }
  destroy $w
  if {$i == {}} { return 0 }
  set cb $suggest(cb)
  $cb set $suggest(text)
  $cb icursor end
  focus $cb
  return 1
}


# ::utils::history::SetCombobox
#
#   Refills the history list of the combobox associated with the specified
#   history key, if there is one.
#
proc ::utils::history::RefillCombobox {key} {
  variable comboboxWidget

  set cbWidget [GetCombobox $key]
  if {$cbWidget == ""} { return }

  # If the combobox widget is part of a dialog which is generated as needed,
  # it may not exist right now:
  if {! [winfo exists $cbWidget]} { return }

  $cbWidget delete 0 end
  set entries [GetList $key]
  $cbWidget configure -values $entries
}


# ::utils::history::Save
#   Saves the combobox history file, reporting any error in a message box
#   if reportError is true.
#
proc ::utils::history::Save {{reportError 0}} {
  variable listData

  set f {}
  set filename [scidConfigFile history]

  if  {[catch {open $filename w} f]} {
    if {$reportError} {
      tk_messageBox -title Scid -type ok -icon warning \
          -message "Unable to write file: $filename\n$f"
    }
    return
  }

  puts $f "# $::scidName combobox history lists\n"
  foreach i [lsort [array names listData]] {
    puts $f "set ::utils::history::listData($i) [list $listData($i)]"
  }
  close $f
}
