###
### selectcolor.tcl 
### A cut-down version of BWidget's SelectColor dialog (color.tcl) which replaces tk_chooseColor
###
### It is not built into the windows and macOS GUIs, but can be so, or sourced, if desired.
###
### Authors Patrick Götz , Stevenaaus
### BWidget ToolKit Copyright (c) 1998-1999 UNIFIX.  Copyright (c) 2001-2002 ActiveState Corp.

namespace eval ::SelectColor {
  variable baseColors {
    #0000ff #00ff00 #00ffff #ff0000 #ff00ff #ffff00
    #000099 #009900 #009999 #990000 #990099 #999900
    #000000 #333333 #666666 #999999 #cccccc #ffffff
  }
  # User colours are ::bwidgetBackgrounds, which is saved in options.dat

  variable w .selectColor
  variable hsv {0 0 0}
  variable color #000000
  variable entry {}
  variable sel -1
  variable result {}
}

### if {$unixOS && !$macOS} 

# Redefine the sad Linux color selector procedure to something useful S.A
proc tk_chooseColor {args} {
  set color #ffffff
  set title Colour
  set parent .
  foreach {opt val} $args {
    switch -- $opt {
      -initialcolor { set color $val }
      -title        { set title $val }
      -parent       { set parent $val }
    }
  }
  return [::SelectColor::dialog $color $title $parent]
}

proc ::SelectColor::dialog {initial title parent} {
  variable w
  variable baseColors
  variable result
  variable sel

  set sel -1
  catch {destroy $w}
  toplevel $w
  wm withdraw $w
  wm title $w $title
  wm resizable $w 0 0
  wm protocol $w WM_DELETE_WINDOW {set ::SelectColor::result cancel}

  ### Palettes: 18 base colours, then 12 user colours

  frame $w.left
  labelframe $w.left.base -text {Base colours} -font fontSmall
  labelframe $w.left.user -text {User colours} -font fontSmall
  set i 0
  foreach c [concat $baseColors $::bwidgetBackgrounds] {
    if {$i < 18} {
      set f $w.left.base.c$i
      set n $i
    } else {
      set f $w.left.user.c$i
      set n [expr {$i - 18}]
    }
    frame $f -width 24 -height 20 -background $c -relief sunken -borderwidth 2 -highlightthickness 2
    grid $f -row [expr {$n / 6}] -column [expr {$n % 6}] -padx 1 -pady 1
    bind $f <Button-1> "::SelectColor::selectSwatch $i"
    bind $f <Double-Button-1> "::SelectColor::selectSwatch $i ; set ::SelectColor::result ok"
    incr i
  }

  labelframe $w.left.sel -text Selection -font fontSmall
  entry $w.left.sel.entry -width 8 -font TkFixedFont -textvariable ::SelectColor::entry
  frame $w.left.sel.color -width 50 -height 24 -relief sunken -borderwidth 2
  pack $w.left.sel.entry -side left -padx 3 -pady 3
  pack $w.left.sel.color -side right -padx 3 -pady 3 -fill y
  bind $w.left.sel.entry <KeyRelease> ::SelectColor::entryChanged

  pack $w.left.base $w.left.user $w.left.sel -side top -anchor w -fill x -pady 2

  ### Hue/saturation square and value (brightness) bar

  labelframe $w.sel -text {Colour selectors} -font fontSmall
  canvas $w.sel.hs -width 200 -height 200 -borderwidth 0 -highlightthickness 0 -relief sunken
  canvas $w.sel.v  -width 15  -height 200 -borderwidth 0 -highlightthickness 0 -relief sunken
  pack $w.sel.hs $w.sel.v -side left -padx 8 -pady 4 -anchor n

  $w.sel.hs create image 0 0 -anchor nw -image [hueImage]
  $w.sel.hs create image 0 0 -image [targetImage] -tags target
  for {set val 0} {$val < 40} {incr val} {
    $w.sel.v create rectangle 0 [expr {5*$val}] 15 [expr {5*$val+5}] -tags val[expr {39-$val}]
  }
  $w.sel.v create polygon 0 0 10 5 0 10 -fill black -outline white -tags target

  bind $w.sel.hs <Button-1>  {::SelectColor::clickHueSat %x %y}
  bind $w.sel.hs <B1-Motion> {::SelectColor::clickHueSat %x %y}
  bind $w.sel.v  <Button-1>  {::SelectColor::clickValue %y}
  bind $w.sel.v  <B1-Motion> {::SelectColor::clickValue %y}

  ### Buttons

  frame $w.buttons
  dialogbutton $w.buttons.ok -text OK -command {set ::SelectColor::result ok}
  dialogbutton $w.buttons.cancel -text [tr Cancel] -command {set ::SelectColor::result cancel}
  pack $w.buttons.cancel $w.buttons.ok -side right -padx 10 -pady 4
  if {[pickMethods] != {}} {
    button $w.buttons.pick -text {Pick from screen} -command ::SelectColor::pickScreen
    pack $w.buttons.pick -side left -padx 4 -pady 4
  }

  grid $w.left    -row 0 -column 0 -sticky n -padx 4
  grid $w.sel     -row 0 -column 1 -sticky ns -padx 4 -pady 2
  grid $w.buttons -row 1 -column 0 -columnspan 2 -sticky ew -padx 4

  bind $w <Return> {set ::SelectColor::result ok}
  bind $w <Escape> {set ::SelectColor::result cancel}

  if {[catch {winfo rgb . $initial}]} {
    set initial #ffffff
  }
  setRgb $initial

  # update idletasks
  placeWinOverParent $w $parent
  wm state $w normal
  # catch {grab $w}
  # focus $w

  set result {}
  vwait ::SelectColor::result
  # catch {grab release $w}
  destroy $w

  if {$result == "ok"} {
    return $::SelectColor::color
  }
  return {}
}

### The hue/saturation image is made once, from 4x4 pixel blocks

proc ::SelectColor::hueImage {} {
  if {[lsearch -exact [image names] selectColorHues] < 0} {
    set data {}
    for {set y 0} {$y < 50} {incr y} {
      set row {}
      for {set x 0} {$x < 50} {incr x} {
        lappend row [format #%02x%02x%02x {*}[hsvToRgb [expr {$x/49.0}] [expr {(49-$y)/49.0}] 0.85]]
      }
      lappend data $row
    }
    image create photo selectColorSmall -width 50 -height 50
    selectColorSmall put $data
    image create photo selectColorHues -width 200 -height 200
    selectColorHues copy selectColorSmall -zoom 4
    image delete selectColorSmall
  }
  return selectColorHues
}

proc ::SelectColor::targetImage {} {
  if {[lsearch -exact [image names] selectColorTarget] < 0} {
    image create bitmap selectColorTarget -data {
      #define target_width 24
      #define target_height 24
      static unsigned char target_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00,
        0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3e, 0xe0, 0x03, 0x3e, 0xe0, 0x03, 0x3e, 0xe0, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x00,
        0x00, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    }
  }
  return selectColorTarget
}

### Choosing a colour

proc ::SelectColor::clickHueSat {x y} {
  variable hsv
  set x [expr {$x < 0 ? 0 : $x > 200 ? 200 : $x}]
  set y [expr {$y < 0 ? 0 : $y > 200 ? 200 : $y}]
  setHsv [expr {$x/200.0}] [expr {(200-$y)/200.0}] [lindex $hsv 2]
}

proc ::SelectColor::clickValue {y} {
  variable hsv
  set y [expr {$y < 0 ? 0 : $y > 200 ? 200 : $y}]
  setHsv [lindex $hsv 0] [lindex $hsv 1] [expr {(200-$y)/200.0}]
}

proc ::SelectColor::entryChanged {} {
  variable entry
  if {![catch {winfo rgb . $entry}]} {
    setRgb $entry 0
  }
}

# Clicking an empty (white) user colour stores the current colour there.
# While a user colour is selected, it follows all changes.

proc ::SelectColor::selectSwatch {i} {
  variable w
  variable sel
  variable color

  if {$sel >= 0} {
    [swatch $sel] configure -highlightbackground [$w cget -background]
  }
  set sel $i
  set f [swatch $i]
  $f configure -highlightbackground black
  set c [$f cget -background]
  if {$i >= 18 && [winfo rgb . $c] == [winfo rgb . white]} {
    setRgb $color
  } else {
    setRgb $c
  }
}

proc ::SelectColor::swatch {i} {
  variable w
  if {$i < 18} {
    return $w.left.base.c$i
  }
  return $w.left.user.c$i
}

# Set the colour from a Tk colour name or #rgb value

proc ::SelectColor::setRgb {c {updateEntry 1}} {
  variable hsv
  set hsv [rgbToHsv {*}[winfo rgb . $c]]
  refresh $updateEntry
}

proc ::SelectColor::setHsv {h s v} {
  variable hsv
  set hsv [list $h $s $v]
  refresh
}

proc ::SelectColor::refresh {{updateEntry 1}} {
  variable w
  variable hsv
  variable color
  variable entry
  variable sel

  lassign $hsv h s v
  set color [format #%02x%02x%02x {*}[hsvToRgb $h $s $v]]

  $w.left.sel.color configure -background $color
  if {$updateEntry} {
    set entry $color
  }
  if {$sel >= 18} {
    [swatch $sel] configure -background $color
    lset ::bwidgetBackgrounds [expr {$sel - 18}] $color
  }

  $w.sel.hs coords target [expr {$h*200}] [expr {(1-$s)*200}]
  set y [expr {(1-$v)*200}]
  $w.sel.v coords target 0 [expr {$y-5}] 10 $y 0 [expr {$y+5}]
  for {set val 0} {$val < 40} {incr val} {
    set c [format #%02x%02x%02x {*}[hsvToRgb $h $s [expr {$val/39.0}]]]
    $w.sel.v itemconfigure val$val -fill $c -outline $c
  }
}

### Pick a colour from anywhere on the screen.
### Tk can't read the screen, so this needs a helper:
###   Wayland: the desktop portal (KDE, Gnome and others) using python3-gi,
###            or KWin's colour picker using gdbus
###   X11:     a screenshot by ImageMagick's "import", which the user clicks on

proc ::SelectColor::pickMethods {} {
  set methods {}
  if {[info exists ::env(WAYLAND_DISPLAY)]} {
    if {[auto_execok python3] != {}} {
      lappend methods Portal
    }
    if {[auto_execok gdbus] != {}} {
      lappend methods KWin
    }
  } elseif {[auto_execok import] != {}} {
    lappend methods X11
  }
  return $methods
}

proc ::SelectColor::pickScreen {} {
  variable w

  # Hide the dialog, so the colour beneath it can be picked
  wm withdraw $w
  update
  after 300

  # A method returns the colour, or {} if the user cancelled. Errors try the next method.
  set color {}
  set error {}
  foreach method [pickMethods] {
    if {![catch {pick$method} color]} {
      set error {}
      break
    }
    set error $color
    set color {}
  }

  wm deiconify $w
  catch {grab $w}
  if {$color != {}} {
    setRgb $color
  } elseif {$error != {}} {
    tk_messageBox -parent $w -icon info -title Scid -message "Could not pick a colour from the screen.\nOn Wayland this needs python3-gi (or KDE), on X11 ImageMagick.\n\n$error"
  }
}

proc ::SelectColor::pickPortal {} {
  set script {
import os, gi
gi.require_version('Gio', '2.0')
from gi.repository import Gio, GLib
bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
token = 'scid%d' % os.getpid()
path = '/org/freedesktop/portal/desktop/request/%s/%s' % (bus.get_unique_name()[1:].replace('.', '_'), token)
def response(conn, sender, obj, iface, signal, params):
    code, results = params.unpack()
    if code == 0:
        print('#%02x%02x%02x' % tuple(round(255 * c) for c in results['color']), flush=True)
    os._exit(0 if code < 2 else 2)
bus.signal_subscribe(None, 'org.freedesktop.portal.Request', 'Response', path, None, Gio.DBusSignalFlags.NONE, response)
bus.call_sync('org.freedesktop.portal.Desktop', '/org/freedesktop/portal/desktop',
    'org.freedesktop.portal.Screenshot', 'PickColor',
    GLib.Variant('(sa{sv})', ('', {'handle_token': GLib.Variant('s', token)})),
    None, Gio.DBusCallFlags.NONE, -1, None)
GLib.MainLoop().run()
}
  # Exit code 0 and no colour means the user cancelled
  set out [exec python3 -c $script 2>@1]
  if {[regexp {#[0-9a-f]{6}} $out color]} {
    return $color
  }
  return {}
}

# KWin returns ((uint32 ARGB,),), and an error if the user cancels

proc ::SelectColor::pickKWin {} {
  if {[catch {exec gdbus call --session --dest org.kde.KWin --object-path /ColorPicker \
                --method org.kde.kwin.ColorPicker.pick 2>@1} out]} {
    if {[string match -nocase *cancel* $out]} {
      return {}
    }
    error $out
  }
  if {![regexp {uint32 ([0-9]+)} $out -> argb]} {
    error $out
  }
  return [format #%06x [expr {$argb & 0xffffff}]]
}

proc ::SelectColor::pickX11 {} {
  close [file tempfile file .png]
  if {[catch {exec import -silent -window root $file} error]} {
    file delete $file
    error $error
  }
  set img [image create photo -file $file]
  file delete $file

  set t .selectColorScreen
  toplevel $t
  wm overrideredirect $t 1
  wm geometry $t [image width $img]x[image height $img]+0+0
  label $t.l -image $img -borderwidth 0 -cursor crosshair
  pack $t.l
  set ::SelectColor::picked {}
  bind $t.l <Button-1> {set ::SelectColor::picked [list %x %y]}
  bind $t.l <Button-3> {set ::SelectColor::picked cancel}
  bind $t <Escape> {set ::SelectColor::picked cancel}
  update
  catch {grab -global $t}
  focus -force $t
  vwait ::SelectColor::picked
  destroy $t

  set color {}
  if {$::SelectColor::picked != "cancel"} {
    set color [format #%02x%02x%02x {*}[$img get {*}$::SelectColor::picked]]
  }
  image delete $img
  return $color
}

### Colour conversions, after tk/demos/tcolor.tcl
### hue, saturation and value are 0-1, red, green and blue are 0-255

proc ::SelectColor::hsvToRgb {h s v} {
  set h [expr {$h >= 1.0 ? 0.0 : $h*6.0}]
  set i [expr {int($h)}]
  set f [expr {$h - $i}]
  set p [expr {round(255*$v*(1 - $s))}]
  set q [expr {round(255*$v*(1 - $s*$f))}]
  set t [expr {round(255*$v*(1 - $s*(1 - $f)))}]
  set v [expr {round(255*$v)}]
  switch $i {
    0 {return [list $v $t $p]}
    1 {return [list $q $v $p]}
    2 {return [list $p $v $t]}
    3 {return [list $p $q $v]}
    4 {return [list $t $p $v]}
    5 {return [list $v $p $q]}
  }
}

# red, green and blue here are 0-65535, as returned by "winfo rgb"

proc ::SelectColor::rgbToHsv {r g b} {
  set max [expr {max($r, $g, $b)}]
  set range [expr {double($max - min($r, $g, $b))}]
  set h 0.0
  set s 0.0
  if {$max > 0} {
    set s [expr {$range/$max}]
  }
  if {$range > 0} {
    if {$r == $max} {
      set h [expr {($g - $b)/$range}]
    } elseif {$g == $max} {
      set h [expr {2 + ($b - $r)/$range}]
    } else {
      set h [expr {4 + ($r - $g)/$range}]
    }
    set h [expr {$h/6.0}]
    if {$h < 0} {
      set h [expr {$h + 1}]
    }
  }
  return [list $h $s [expr {$max/65535.0}]]
}
