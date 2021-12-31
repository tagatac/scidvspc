###
###    inputengine.tcl
###    Namespaces 'ExtHardware' and 'inputengine'
###
###    $Id: inputengine.tcl,v 1.13 2010/03/08 17:39:27 arwagner Exp $
###    Last change: <Mon, 2010/03/08 18:38:38 arwagner ingata>
###    Author     : Alexander Wagner

###    Author     : stevenaaus 2017, 2021, 2022
#
# Handle configuration for Novag and Input Engine, and also Connect for the Input Engine.
# (Connect and addMove for Novag is handled in novag.tcl. Alex's code structure is a little ordinary).

# Engine is connecting (searching the board, initialising)
image create photo tb_eng_connecting -data {
  R0lGODlhFgAWAOfcAAACAAQHAgAKBAARAQESAw0TBwccAQkcAwUfBwQiAwUjBAgnCgwnAwcsCQgt
  Cw4wBxMwDxQxEBNICQtNDQ9QEBNaEg5fDRZcFBNiEAhnFRJsGwxyFxZuHBdvHRxwFhN1GhpxHxV2
  HBl2EhxyIB1zIQ98GBh4Hhx4FRp5Hx55FhN+Ght6IB96FxV/Gw6FFxeAHBuAEyd4HyV5JxGGGRmB
  Hh2BFCB9IyZ6KBuCHx+CFRyDICCDFiN/JRWIGyh8KSp6NR6EISKEGBiJHCWBJx+FIip+KxmKHSeC
  KCGGIyWGGhOQGRuLHiyALSKHJCuDITV6Nh2MHzZ7Ny6BLieIHCSIJSmFKjd8OB+NISONFyWKJhiT
  GyyHLBqUHS2ILTCIJSKQIyaQGS6JLjOGMyqNKR6WHyWSJTGLMCCXICeTJiGYIRqdHCiUJzONMS+R
  LRyeHS2SNSuWKSWaJDaPMy6TNiyXKiabJSCgIDiRNSKhIS+ZLCSiIiajIyyfKTCcNy6gKjaZPEKT
  PymlJj+XOyunJzKkLkaXQyqpMjSlLzyfQUCgOy2rNDanMTioMkicTTapOzqqNEahSkOlRz2sNjut
  Pz6tN1WfUVagUketQD+xQkCyQ1KpTEqwQk2rWU6sWky1Tki4SWOnYE62T2CrXE+3UFa0TlC4UVyw
  YFK7U1i4WFy5U1m6WXCvbmC6YWS4Zm6za2S7XF/AX2LCYW+9c36zgG/AaH23fXq6eG/CcHjCcou4
  hoq/i3zKf4vAjITJf4PLh4nOhJHMkJfHm5POk5XPlJrVmaPRnqTQqp3YnJ/anqbZnqbdp6feqKnf
  qazirbHitMfew8DlwMHrvsDtxtTn09LqztLy1Njw1NPz1dny1eHw4+338v7//P//////////////
  ////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////yH+FUNyZWF0ZWQgd2l0aCBU
  aGUgR0lNUAAh+QQBCgD/ACwAAAAAFgAWAAAI/gD/CRwoEEAAAwoMCCDIkKCACFEgfcp0aRIaDg4W
  NhQYIMqpUp5Q7fp16xKjQB4CbCzQiZUyZ8yQJSPmi1aqTYGEEGAooFMsZNiqRWs2zJWoVq9CMfKz
  Q6VAAU9iOcN2TZozYJa8gHLS6ZIkPXYsaISgatg0atCYDbNFY9q2VUAS+fET5wwDgT9CFSsECJAc
  OVucGNv2VkeeOHXSXPgXABGqWoWeSZ78TBvhVTrQYFEyA8GBSb5mWSJMuvRlJ2OuXFGgYNIwXVsq
  yZ5Nu5IlKWKmQFFwgBIuXU5qC6/kREqVKQoCGPIky8kq09Bp+OBBRAGAK4xYmaryfFs2XuDBpuvS
  BcOKDRYH/nnwUypUJBzdgzkxs+VIDCkrYuCgIJDBnksmJaLDc6s4IUgbY1QRAwnnKfBUBnss5Qcd
  OshhYBtTvLBCByC0IIFGjAERBx9xoNHGHXOMkYQNIYAwQggbIMAQASrEkQYXV1CBRBUokBCCCS14
  4MBGAVSABRhYTKGDCiuwkMMJEqS30T8CLFABDUsY0UMQIUiQQAFTMoQAawokIONGAQEAOw==
}

# Engine is disconnected (default)
image create photo tb_eng_disconnected -data {
  R0lGODlhFgAWAOeWAAEBAQICAggICAkJCQoKCg4ODg8PDxMTExoaGhsbGxwcHB0dHSAgICIiIiMj
  IyoqKisrKywsLC0tLS4uLjk5OTo6Ojs7Ozw9PUhISE1NTU5OTl1dXWJjY2NjY2RkZGVmZmZmZmdn
  Z2doaGlpaWlrbmlsbmptcGttb2xucG9vb29wcG9wcnFxcXBydXFyc3JycnFzc3Nzc3N0dnJ1d3V1
  dnV2dnV2d3Z3eHd3eHh4eXh5e3h6fHp6ent7e3t7fHp8fnx8fHx9fX19fnx+f31+fn1+gH5+fn9/
  f4CAgYGBgYGChIKDhIODg4OEhYOEhoWFhYWGh4WGiPxUVIaHiIeHh4eHiIeIiIiJiYmJiYmKi4qK
  i4yMjIyNjY6Ojo+Pj4+PkI+QkZGRkZKSk5SUlJWVlZaWlpeXl5iYmJiYmZqampubm5ycnJ2dnZ6e
  np+fn6CgoKKioqSkpKampqenp6mpqaqqqqysrK6urq+vr7CwsLGxsbKysrW1tba2tre3t7i4uLm5
  ubu7u7+/v8DAwMHBwcLCwsXFxcfHx8zMzM3Nzc7OztTU1NbW1tfX19jY2Nzc3N/f3+Dg4OLi4ufn
  5+np6ff39///////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////yH+FUNyZWF0ZWQgd2l0aCBU
  aGUgR0lNUAAh+QQBCgD/ACwAAAAAFgAWAAAI/gD/CRwoMMCABA0SECDIkCCBCTHG+JkIZ8sICAsb
  ChTAgg4cRIgmvnnDZo0HARoPtOlTCFEjkHvm3DFjZk0PAwwFsAHEaFGjRocmzplD0wyZFCgFEmDx
  55EUKYeiTvQzx42ZMme0aMgoAY8jSk+hxjxkiapVMmGSNBD4Io8kQYLCTrVU1o0XL1+qZPg3gAyf
  SIKiho0Kia4fM160TImxAAEdRIkC0bUUdjJdxFOgGGnQoM4iQlMnyp065wgVJUAaIJBjSBGe0H5G
  Uz0ypAiPBgLg2BnUJ06l37/DAj9yRAeOBgCowPmT542a35PghoVr2oaIBf9AvNFTx82YOZUsjc1B
  gyUs8SM1Lgh8sIaOHDZktswRbwYJlLAmXITA/o/ABjNupCGGFWYQZQYVVBzxVAkwVJDRPwKk0AUZ
  YFwRBE0JEncCCShwoABDBXTwBBdZNIFEDlT8MEMLK7jAQQQaCYBBD0ws4UQUS+wgAw0fUJCARkox
  gIEKQhDhww0fWKDAg0AKtIADUDrAX0MBAQA7
}

# Engine reported an error
image create photo tb_eng_error -data {
  R0lGODlhFgAWAOf4AAYAAAcAAAkBAA8AABABARYAAyIBASUAAxkFCScBACoAAC0AAC0AAS4BAjIA
  ADMAADgAAjkAAzwBADsHCzwIDF4AAF8AAGEAAmQBAHQAAXYAAHcAAH0AAIIAAIQDAIsAA4sADI0B
  BY0BDY4DBpYAA5gAAJgABJAFAJkCAKAAAZkDBaEAApgDDaIAA6AACYgMC6MCBJIKCZEKEKsAAJsH
  B6wAAaUEAIsPDa0AAaUEBa4AAq4BCrQAAJQOErADA7cAALUABrgAALkAALcAB4YWF50MEK8EC7kA
  CbEGBJYSE8AABKgMB8MAAMIABcQAAMMABrwFAsYBALwFC8UBCKkPEL0IA8cEAM4ABLMMDs8ABKsR
  EYweIbUOB88ADtAAD44fHcgHAKMWGpwaGI8gHpUdILYRCMgICookIrYREMoLAZAiJJcfIckLC4wm
  JJ8eIJIkJqcbHcsOAsERD8kMFJQlIZMlJssODJojJJQmJ8ITEJwkH48qK8wRDaofIM0TDtYOE58o
  J6ApKKcmJc4WF6gnJpUvL5stLK8lI9AYEJYwMM8YGM8YH8gdHJ8wL5ozM9IcGskfI8EjJtMeG7Ur
  LcMlJ6kxLps2OtMeIpg5OtQgIs0kJdUiI5s8PNcjJM8nJ9YkKtglJbwyMtApLp8/P684OdIqKcot
  MdMrKqs8PsUyMK0+P88yL9cwLL08O8M6PdE1N9I2N9o0Nds1O9U5Od02N9A9OMk/QcBHRctCQ8ZF
  Qs5ERahWWN1BQKZaWdhFRKdbWsFQT81LTahcW9tHRslPUtxIR6peXd1JSONHSsFXWLBjYuJOS+FO
  UcFeYdpXV99bW8RnZsVoZ+JdXONfXdxvbr9/feVwcuZxc8d/geZ/gOiAgciOkOSFiM2TlemIhuaN
  jeuLj9WbnO6bmc6pqu6cn/Ceoeyhoe2iou6jo++kpPClpdS2te64uN3AvvS3ufK9vOfQzPHMzPjL
  zvXP0PfR0vjS0+3g4f7//P///////////////////////////////yH+FUNyZWF0ZWQgd2l0aCBU
  aGUgR0lNUAAh+QQBCgD/ACwAAAAAFgAWAAAI/gD/CRwoMEAAAw0MCCDIkOAACnhCffIEaRGaFxEG
  NBwYAA8tWKFaQYOWLFIePx4CbESAi9m3cI529eq1602uUl50FGAYwNYyc/HWCbtH9N4bYLAkgbGh
  UuAAOsfM0Ys3zljRe46ClXrEhgkHjf8m0PI2rx67bnuKwnOka5EXM0+qPBA4JpY6ee/MYSNStF2j
  Woj8TFGihMM/AZR8nXN3zhu0tETboXrl54qSIENqJDgA6po6cd6oJdvSV5WorkKQ8ODRQAErb+ZC
  O7u1pR3RbaQ8sXEipEYN1gcePctmTZquVrWJVqvEqEoVIDBa1Ggw4A+rZ9OOyTIFyPa95YuqwPBY
  QmOFjQYBsHRhxkwWRTiJRo3aMqlM5hUsVKBI8O8Fm1WrsDIII2FgUk01ZwhixBIunMDCDhgIJMEV
  oHwySB5DiOBdNYBokQMJMZCAggICCcBBHFzlgYaGytERRgwveLBCBWD9Q0AOTrDxRBliKGOPPZYk
  4oYIIrBwwgEMFbBCFVIcoYUbZOBhCRk9iHiCBBsFkEEQQhhBRR+BuFFECipcgORG/wTwQAYt6IDF
  EkucgIECC6FJ0AEN5MkAiRsFBAA7
}

# Engine is connected, communication established
image create photo tb_eng_ok -data {
  R0lGODlhFgAWAOe/AAABBAAHCgIIDAMKDQYMDwQOFQoQEgkVHwoWIAsXIA0XIQ4ZIxAbJBEbJQ4d
  KxEgLxQjMRQkMhUkMxYlNB43UB84USI7VCQ9VSdGZCpIZy1LaihNcSxRdS9UeDFVeSxYgS1ZgjRX
  fC5agy9bhDBbhTJchjNdhzReiD9cezVfiTtegzxfhEFefTdhi0ZdeD1ghThijDJkkz5hhjljjTNl
  lDpkjjVmlUBjiDtlj0FkiTxmkDdol0Jlij5nkThpmT9okkRnjDlqmkBpk0VojTprm0FqlDtsnD1t
  nT5unkRtlz9vn0pskkBwoEttkzpyp0FxoUdwmkJyolJuj0hxm0Nzo1NvkEpynk9xl1BymEB3rVZy
  k0F4rlJ0mlN1m092okp5qVV3nVZ4nlJ5pUx7q1t3mEt9p0d9s018rVR7p1l6oFR/pF97nFB/sGB8
  nVx9pFKAsUyCuFh/q11+pVSCs1qBrWCBqGWBommAnWGCqWaCo2KDqliGt2OEq12IrV+Gs1mKtVyK
  vGKJtl+MvnKJpnSMqGmPvGqQvWiSuGuRv3CRuW6VwnKXt3CXxHGbwXKcw3mZwoKZtnqfwIGdwHug
  wYKewYKivoCkxoWlwYunvYaqzJGoxo2typyrvpCvzJWty5qtxpiwzpywyJS00Juy0Ju3zqK1zqW1
  yKO2z6C31py82aO62aS/1qXA16bB2azA2bTAzrLB1a7C27PD1rDE3a/G2LTE177K2MTQ3sbS4MfT
  4czV3cvX5dfg6OXq7f7//P//////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////yH+FUNyZWF0ZWQgd2l0aCBU
  aGUgR0lNUAAh+QQBCgD/ACwAAAAAFgAWAAAI/gD/CRwoEECAAw0OBCDIkKCACSzSCAIEaI8VDxEG
  NBwIwIWhQ4USZfL0KNAYNhwEbDSQp1GqWatUnQqFqQ0JMHGKFGAIoA+jWLty0RKlaVGbNb1yuBkT
  Y6FAASwYtQoaaxSlRnlu9PL1SkaaLxtU/pOAyFVQWqAsJdIj45Wvt3mWeEHyQCCKQ61utfIUKVGf
  JoTevu11YwoVDf8CiHnEqtYlSIfoqJGxVTCvHEuY9FCgANAmV59UpMkjR0YpwVyBcIGSBEmDBoBE
  kSJEaE0VLVJQw1Ih50sQH0QS7qGECciupCtYCX6lQg+aIz10EHEwYA4gQkve2sr9FpYMOmWeswRJ
  EcNGAwBW5nThJHjXW14q6qBBUmRGihQqFPzTMEfNDFuo9cKDGGNQgYQNJ5gQgwUCScAGH22o4NZb
  dmBRBhVE2FBCCAk68FQIZrwhhgqf+FJKRFME0YIJIoQAAwVi/QNAD1+U8YUKplTRBRQ+wLBhCCSE
  kABDBZjAhIEmqFCfCiqUcAIMJESwEQAYIPFEETb0cJ8JM5RAwQEbPdUABtIR4YMNJFigAABhMqTA
  a6/pt1FAADs=
}

image create photo tb_eng_dgt -data {
  R0lGODlhFgAWAKEDAAAAAAAA//8AAP///yH+FUNyZWF0ZWQgd2l0aCBUaGUgR0lNUAAh+QQBCgAD
  ACwAAAAAFgAWAAACLZyPqcvtD02YMyJarcR60z5gwQOUJWieXQqgqWuCoEDTcy3ctW53eC4LCoeD
  AgA7
}

image create photo tb_eng_query -data {
  R0lGODlhFgAWAOe+AAEBAQICAggICAkJCQoKCg4ODg8PDxMTExoaGhsbGxwcHB0dHSAgICIiIiMj
  IyoqKisrKywsLC0tLS4uLjk5OTo6Ojs7Ozw9PUhISE1NTU5OTl1dXWJjY2NjY2RkZGVmZmZmZmdn
  Z2doaGlpaWhrbmlrbmlsbmptcGttb2xucG9vb29wcG9wcnFxcXBydXFyc3JycnFzc3FzdXNzc3F0
  dXF0dnN0dnJ1d3V1dnN2d3R2eHV2dnV2d3Z3eHd3eHZ4enh4eXh5e3h6fHp6enl7fHp7e3t7e3t7
  fHp8fnx8fHt9f3x9fX19fnx+f31+fn1+gH5+fn9/f4CAgICAgX+BgoCBg4GBgYGChIKDhIODg4KE
  hYOEhYOEhoSEhIWFhYWGh4WGiIaGhoaHiIeHh4eHiIeIiIiJiYiJiomJiYmKi4qKioqKi4uLi4uL
  jIyMjIyNjY2NjY2Oj46Ojo+PkI+QkI+QkZCQkJGRkZGSkpKSkpKSk5KTk5OTk5OTlJSUlJSUlZWV
  lZWVlpaWlpeXl5iYmJiYmZmZmZqampubm5ycnJ2dnZ6enp+fn6CgoKGhoaKioqOjo6SkpKWlpaam
  pqioqKmpqaqqqqysrK2tra6urq+vr7CwsLGxsbKysrW1tba2tre3t7i4uLm5ubu7u7y8vL29vcDA
  wMHBwcLCwsXFxcbGxsfHx8zMzM3Nzc7OztPT09TU1NbW1tjY2Nzc3N3d3d7e3uDg4OLi4unp6evr
  6+zs7PDw8PPz8/X19f//////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////yH+FUNyZWF0ZWQgd2l0aCBU
  aGUgR0lNUAAh+QQBCgD/ACwAAAAAFgAWAAAI/gD/CRwoMMCABA0SECDIkCCBCTP8UJok6ZGbERAW
  NhQooEWlR4oMLXLUqJGiRB4EbDyw6BMqP5pk9eolK5OfSImMGGAoQJGoWLA8+RpKdNUdRYBUqBRI
  oEWoWbpeASJK9Y4gQms0aJQQkxeuVo0QeZFyiWiePIDuWGkgEMamW7tqpeIkKRGcSETt8Okzh0yG
  fwMAeaplixUpTpUOSck1VFYXQXvWiJmxAEElVrRajeqE6dEYWUN7hfETiI6YL1AaNLAES9apTpco
  IVpE1I8dP3jWYLmSpAGCSalcjcJECdKgVENTsRHUJ46YJk+GNBDw6JKpT5UiOfID2heoOX/swJyp
  8iOIjwYAxjwKtSlSWDtq2Ljp0qZOGixEaPAQseAfiEacWMIIIn5YoYovvRSCRhpaICGDDDtcINAD
  iVQyCVJuuEGULFN8oUQNJ7wQQn//ELDBIIwcokcZbWwYBRU6kGBCDBVo9I8AKsgBSB1mLFEKgog4
  kQMKJaTAgQIMFdCBF2+kscUUQBSBxA0usPACBxFsJAAGRmSBBRdgYCGEDTh8QEECGzHFAAYrMOHE
  ET18YIECNqYp0AIO5OkAiQ0FBAA7
}

namespace eval ExtHardware {

  # These defaults may be overwritten by reading hardware.dat (below)
  set engine     dgtdrv2.i686
  set port       /dev/ttyUSB0
  set param      la

  # Hardware configured by default:
  #  1 : Novag Citrine
  #  2 : Input Engine
  if {![info exists hardware]} {
    set hardware 1
  }

  if {![info exists bindbutton]} {
    set bindbutton ::novag::connect
  }
  set showbutton 0

  proc saveHardwareOptions {} {
    if {[catch {open [scidConfigFile ExtHardware] w} optionF]} {
       tk_messageBox -title "Scid: Unable to write file" -type ok -icon warning \
	  -message "Unable to write options file: [scidConfigFile ExtHardware]\n$optionF"
    } else {
       puts $optionF "# $::scidName options file"
       puts $optionF "# Version: $::scidVersion, $::scidVersionDate"
       puts $optionF ""

       foreach i { ::ExtHardware::engine
		   ::ExtHardware::port
		   ::ExtHardware::param
		   ::ExtHardware::hardware
		   ::ExtHardware::showbutton
		   ::ExtHardware::bindbutton } {
	  puts $optionF "set $i [list [set $i]]"
       }
      close $optionF
      set ::statusBar "External hardware options were saved to: [scidConfigFile ExtHardware]"
    }
  }

  ### Set the hardware connect button image

  proc HWbuttonImg {img} {
    if {$::ExtHardware::showbutton} {
      .main.button.exthardware configure -image $img -relief flat -width 30 -height 30
    }
  }

  ###  Configure both novag and 'input engine'

  proc config {} {
    global ::ExtHardware::port ::ExtHardware::engine ::ExtHardware::param ::ExtHardware::hardware tr

    ::ExtHardware::HWbuttonImg tb_eng_query

    set w .exthardwareConfig
    if { [winfo exists $w]} {
      raiseWin $w
      return
    }

    toplevel $w
    wm state $w withdrawn
    wm title $w $tr(ExtHWConfigConnection)

    label $w.lport -textvar tr(ExtHWPort)
    entry $w.eport -width 40 -textvariable ::ExtHardware::port

    label $w.lengine -textvar tr(ExtHWEngineCmd)
    entry $w.eengine -width 40 -textvariable ::ExtHardware::engine

    label $w.lparam -textvar tr(ExtHWEngineParam)
    entry $w.eparam -width 40 -textvariable ::ExtHardware::param

    label $w.options -textvar tr(ExtHWHardware)
    
    checkbutton $w.showbutton -textvar  tr(ExtHWShowButton) -variable ::ExtHardware::showbutton -command {
      if {$::ExtHardware::showbutton} { 
	 pack .main.button.space4 .main.button.exthardware -side left -pady 1 -padx 0 -ipadx 2 -ipady 2 
      } else { 
	 pack forget .main.button.space4 .main.button.exthardware
      }
    }

    # Add a new radio button for subsequent new hardware here:

    radiobutton $w.novag -textvar tr(ExtHWNovag) -variable ::ExtHardware::hardware -value 1 -command {
       set ::ExtHardware::bindbutton ::novag::connect
       .exthardwareConfig.eengine configure -state disabled
       .exthardwareConfig.eparam  configure -state disabled
    }

    radiobutton $w.inputeng -textvar tr(ExtHWInputEngine) -variable ::ExtHardware::hardware -value 2 -command {
       set ::ExtHardware::bindbutton ::inputengine::connectdisconnect
       .exthardwareConfig.eengine configure -state normal
       .exthardwareConfig.eparam  configure -state normal
    }

    if { $::ExtHardware::hardware == 1 } {
       .exthardwareConfig.eengine configure -state disabled
       .exthardwareConfig.eparam  configure -state disabled
    }

    grid $w.options    -sticky ew    -row 0 -column 0
    grid $w.novag      -sticky w     -row 0 -column 1
    grid $w.inputeng   -sticky w     -row 1 -column 1

    grid $w.lport      -sticky ew    -row 2 -column 0 
    grid $w.eport                   -row 2 -column 1 -padx 10

    grid $w.lengine    -sticky ew    -row 3 -column 0
    grid $w.eengine                 -row 3 -column 1 -padx 10

    grid $w.lparam     -sticky ew    -row 4 -column 0 
    grid $w.eparam                  -row 4 -column 1 -padx 10

    grid $w.showbutton -sticky w     -row 5 -column 1

    grid [frame $w.buttons]         -row 6 -column 0 -columnspan 2 -pady 10 

    # Connect 
    dialogbutton $w.buttons.connect -textvar tr(FICSConnect) -command {
       ::ExtHardware::saveHardwareOptions
       destroy .exthardwareConfig
       $::ExtHardware::bindbutton
    }

    dialogbutton $w.buttons.help -textvar tr(Help) -command {helpWindow HardwareConfig}

    dialogbutton $w.buttons.close -textvar tr(Close) -command {
      ::ExtHardware::saveHardwareOptions
      ::ExtHardware::HWbuttonImg tb_eng_disconnected
      destroy .exthardwareConfig
    }

    pack $w.buttons.connect $w.buttons.help $w.buttons.close -side left -padx 20

    bind $w <F1> {helpWindow HardwareConfig}

    update
    placeWinOverParent $w .
    wm state $w normal
  }

} ; # namespace ExtHardware

set scidConfigFiles(ExtHardware) hardware.dat

frame .main.button.space4 -width 15

# Same padding as '.main.button.$i configure' in main.tcl
button .main.button.exthardware -image tb_eng_disconnected -relief flat -border 1 \
  -highlightthickness 0 -takefocus 0 -width 30 -height 30
bind .main.button.exthardware <Button-3> ::ExtHardware::config

# Source ExtHardware options file

if {[catch {source [scidConfigFile ExtHardware]} ]} {
  # don't spam splash because this file normally doesnt exist
  # ::splash::add "Unable to read External Hardware options file: $scidConfigFiles(ExtHardware)"
} else {
   if {$::ExtHardware::showbutton} {
      pack .main.button.space4 .main.button.exthardware -side left -pady 1 -padx 0 -ipadx 2 -ipady 2
   }
  ::splash::add "External hardware configuration found and loaded."
}

.main.button.exthardware configure -command $::ExtHardware::bindbutton

namespace eval inputengine {

  set engine     dgtdrv2.i686
  set port       /dev/ttyUSB0
  set param      la

  set InputEngine(pipe)     ""
  set InputEngine(log)      ""
  set InputEngine(logCount) 0
  set InputEngine(init)     0
  set connectimg            tb_eng_ok
  set MovingPieceImg        $::board::letterToPiece(.)80
  set MoveText              "     "

  set NoClockTime           --:--
  set StoreClock            0

  set WhiteClock            $::inputengine::NoClockTime
  set BlackClock            $::inputengine::NoClockTime
  set oldWhiteClock         $::inputengine::NoClockTime
  set oldBlackClock         $::inputengine::NoClockTime
  set toMove                White

  font create moveFont -family Helvetica -size 56 -weight bold

  proc createConsoleWindow {} {
    set w .inputengineconsole
    if { [winfo exists $w]} { destroy $w }

    toplevel $w

    wm title $w [tr IEConsole]

    scrollbar $w.ysc     -command { .inputengineconsole.console yview }
    text      $w.console -height 5 -wrap word -yscrollcommand "$w.ysc set"

    label     $w.lmode   -text [tr IESending]

    ::board::new $w.bd 35
    $w.bd configure -relief solid -borderwidth 1

    label     $w.engine      -text "$::ExtHardware::engine $::ExtHardware::port $::ExtHardware::param"

    radiobutton $w.sendboth  -text [tr Both]  -variable send -value 1 -command {::inputengine::sendToEngine sendboth}
    radiobutton $w.sendwhite -text [tr White] -variable send -value 2 -command {::inputengine::sendToEngine sendwhite}
    radiobutton $w.sendblack -text [tr Black] -variable send -value 3 -command {::inputengine::sendToEngine sendblack}

    button $w.bInfo          -text Info               -command {::inputengine::sendToEngine sysinfo}

    ### TODO rotate does not work yet
    button $w.bRotate        -text [tr IERotate]      -command {::inputengine::rotateboard}

    button $w.bSync          -text [tr IESynchronise] -command {::inputengine::synchronise}
    button $w.bClose         -text [tr Close]         -command "
      catch {::inputengine::disconnect}
      bind $w <Destroy> {}
      destroy $w"

    # Buttons for visual move announcement
    button $w.bPiece -image $inputengine::MovingPieceImg
    button $w.bMove  -font moveFont -text  $inputengine::MoveText

    #SA ::inputengine::setPieceImage bq80

    $w.bPiece configure -relief flat -border 0 -highlightthickness 0 -takefocus 0
    $w.bMove  configure -relief flat -border 0 -highlightthickness 0 -takefocus 0

    # Buttons for clock display
    button $w.wClock -text  $inputengine::WhiteClock
    button $w.bClock -text  $inputengine::BlackClock
    $w.wClock configure -relief flat -border 0 -highlightthickness 0 -takefocus 0
    $w.bClock configure -relief flat -border 0 -highlightthickness 0 -takefocus 0


    # Store the time as comment
    checkbutton $w.bStoreClock -text "Store Clock" -variable ::inputengine::StoreClock

    grid rowconfigure $w 0 -weight 1
    grid rowconfigure $w 11 -pad 10
    grid columnconfigure $w {2 3 4 5 6} -weight 1
    grid columnconfigure $w 11 -weight 1
    grid columnconfigure $w 12 -weight 0

    grid $w.console    -sticky nsew  -column 0  -row 0 -columnspan 12 -padx 5 -pady 3
    grid $w.ysc        -sticky nse    -column 12 -row 0

    grid $w.engine     -sticky ewns   -column 0  -row 1 -columnspan 9

    grid $w.lmode      -sticky ew    -column 0  -row 2 -padx 12
    grid $w.sendboth   -sticky e     -column 2  -row 2 
    grid $w.sendwhite               -column 4  -row 2 
    grid $w.sendblack  -sticky w     -column 6  -row 2 

    grid $w.bInfo      -sticky ew    -column 0  -row 3 -padx 12
    grid $w.bRotate   -sticky ew    -column 0  -row 4 -padx 12
    grid $w.bSync      -sticky ew    -column 0  -row 5 -padx 12
    grid $w.bStoreClock -sticky ew   -column 0  -row 6 -padx 12
    grid $w.bClose     -sticky ew    -column 0  -row 11 -padx 12

    grid $w.bPiece     -sticky nwes  -column 2  -row 3 -rowspan 7 -columnspan 3
    grid $w.bMove      -sticky nwes  -column 5  -row 3 -rowspan 7 -columnspan 3

    grid $w.wClock     -sticky nwes  -column 9 -row 11 -columnspan 7
    grid $w.bClock     -sticky nwes  -column 9 -row 1  -columnspan 7

    grid $w.bd         -sticky nw    -column 9  -row 2 -rowspan 9 -columnspan 7 -padx 12

    ### General purpose command entrybox - S.A

    frame $w.comms
    grid $w.comms -sticky ew -column 1 -columnspan 13 -row 12 -pady 10

    pack [entry $w.comms.command -width 60] -side left -padx 20
    pack [button $w.comms.send -text Send -width 10 -command {
      ::inputengine::sendToEngine [.inputengineconsole.comms.command get]
      .inputengineconsole.comms.command delete 0 end
    }] -side right -padx 30
    bind $w.comms.command <Return> "$w.comms.send invoke"

    bind $w <Control-q> "$w.bClose invoke"
    bind $w <Destroy> "
      bind $w <Destroy> {}
      $w.bClose invoke
    "

    bind $w <F1> {helpWindow InputEngine}
  }

  proc connectdisconnect {} {
    if {$::inputengine::InputEngine(pipe) == ""} {
      createConsoleWindow
      ::inputengine::resetEngine
      ::inputengine::connect
    } else {
      ::inputengine::disconnect
    }
  }

  proc connect {} {
    global ::inputengine::InputEngine ::inputengine::engine ::inputengine::port ::inputengine::param

    set engine $::ExtHardware::engine
    set port   $::ExtHardware::port
    set param  $::ExtHardware::param

    ::ExtHardware::HWbuttonImg tb_eng_connecting
    set command "$engine $port $param"

    if {[catch {set InputEngine(pipe) [open "| $command" r+]} result]} {
      ::ExtHardware::HWbuttonImg tb_eng_error
      tk_messageBox -title "Scid: Input Engine" -icon warning -type ok \
          -message "[tr IEUnableToStart]\n$command"
      ::inputengine::resetEngine
      return
    }

    ::inputengine::Init
  }

  proc disconnect {} {
    set ::inputengine::connectimg tb_eng_connecting 
    ::inputengine::sendToEngine stop
    ::inputengine::sendToEngine quit
    set ::inputengine::connectimg tb_eng_disconnected
  }

  proc logEngine {msg} {
    set t .inputengineconsole.console
    $t insert end "$msg\n"
    $t yview moveto 1
  }

  proc sendToEngine {msg} {
    ::inputengine::logEngine "> $msg"
    puts  $::inputengine::InputEngine(pipe) $msg
    flush $::inputengine::InputEngine(pipe)
    # flushing here necessary ?
  }

  proc Init {} {
    global ::inputengine::InputEngine
    set pipe $::inputengine::InputEngine(pipe)

    # Configure the pipe and intitiate the engine
    set pipe $::inputengine::InputEngine(pipe)
    fconfigure $pipe -buffering full -blocking 0
    # register the eventhandler
    fileevent  $pipe readable "::inputengine::readFromEngine"

    ::inputengine::newgame
  }

  proc resetEngine {} {
    ::ExtHardware::HWbuttonImg tb_eng_disconnected

    set ::inputengine::InputEngine(pipe)     ""
    set ::inputengine::InputEngine(log)      ""
    set ::inputengine::InputEngine(logCount) 0
    set ::inputengine::InputEngine(init)     0
  }

  #----------------------------------------------------------------------
  # rotateboard()
  #    Rotates the board, ie. exchanges a1 and h8
  #----------------------------------------------------------------------
  proc rotateboard {} {
    global ::inputengine::InputEngine
    set pipe $::inputengine::InputEngine(pipe)

    # rotate the graphical boards
    toggleRotateBoard
    ::board::flip .inputengineconsole.bd

    ::inputengine::newgame
    # rotate the board for the input engine
    ::inputengine::sendToEngine "rotateboard"
    ::inputengine::synchronise
  }

  #----------------------------------------------------------------------
  # newgame()
  #    Handle NewGame event from board
  #----------------------------------------------------------------------
  proc newgame {} {

    # Ask the user to save the current game
    ::game::Clear
    sc_game tags set -event "InputEngine Input"
    sc_game tags set -date [::utils::date::today]
  }

  proc endgame {result} {
    set filternum [sc_filter first]

    logEngine "  info End Game $filternum: $result"

    sc_game tags set -result $result
    gameAdd
  }

  #----------------------------------------------------------------------
  # synchronise()
  #    read board position and set scid's representation accordingly
  #----------------------------------------------------------------------
  proc synchronise {} {
    logEngine "  info Sync called"
    set ::inputengine::InputEngine(init) 0

    ::inputengine::sendToEngine "getposition"
    ::inputengine::sendToEngine "getclock"
  }

  #----------------------------------------------------------------------
  # readFromEngine()
  #     Event Handler for commands and moves sent from the input
  #     engine, implements input engine protocol
  #----------------------------------------------------------------------
  proc readFromEngine {} {
    global ::inputengine::InputEngine ::inputengine::connectimg

    set pipe $::inputengine::InputEngine(pipe)
    set line [string trim [gets $pipe]]

    # Close the pipe in case the engine was stopped
    if {[eof $pipe]} {
      catch {close $pipe}
      ::inputengine::resetEngine
      return
    }

    switch -regexp -- $line \
        "^move *" {
          set m [string range $line 5 end]

          set s1 [string range $m 0 1]
          set s2 [string range $m 2 end]
          if {$s1 == "0-"} {
            # casteling must not be rewritten
            set m "$s1$s2"
          } else {
            set m "$s1-$s2"
          }

          logEngine "$line"

          if {[catch {sc_move addSan $m}]} {
             ::utils::sound::PlayMove sound_start
             logEngine "  info Illegal move detected!"
             logEngine "  info Ignoring: $m"
             .inputengineconsole.bPiece configure -background red
             .inputengineconsole.bMove  configure -background red -text $m
          } else {

            .inputengineconsole.bPiece configure -background green
            .inputengineconsole.bMove  configure -background green -text $m

             updateBoard -animate
             updateBoard -pgn
             ::inputengine::sendToEngine "getposition"
             ::inputengine::sendToEngine "getclock"
          }
        } \
        "info *" {
          logEngine "< $line"
          set event [string range $line 5 end]
          switch -regexp -- $event \
          "string ERROR *" {
            set err [string range $event 7 end]
            tk_messageBox -title "Scid: Input Engine" \
            -icon warning -type ok -message "Engine $err"
            catch {close $pipe}
            ::ExtHardware::HWbuttonImg tb_eng_error
            return
          } \
          "string Chessboard found and initialised*" {
            # switch to xboard mode and disable move
            # announcments by the driver engine
            ::inputengine::sendToEngine "xboard"
            ::inputengine::sendToEngine "announce"
          } \
          "Engine mode    : xboard*" {
            ::inputengine::sendToEngine "getposition"
            ::ExtHardware::HWbuttonImg $inputengine::connectimg
          } \
          "string FEN *" {
            set InputEngine(init) 0
            # The first FEN string is always sent as
            # info string FEN ...
            # as this is compatible with both UCI and xboard.
            # At this stage the engine is not set to xboard mode
            # yet, so this signals a new program startup
            # accordingly.
          } \
          "FEN *" {
            set fenstr [string range $event 4 end]
            set fenstr [string trim $fenstr]
            if { $::inputengine::InputEngine(init) == 0 }  {
              # Initialise scids representation with the FEN
              # delivered by the external board.
              catch {sc_game startBoard $fenstr}
              updateBoard -pgn
              set InputEngine(init) 1
            } else {
              # Compare the internal representation to the
              # board each time a FEN string arrives from the
              # driver.
              #
              # Do not check the whole FEN, as the external
              # board can not know anything about e.p. or O-O
              # capabilities. Strip them off and compare just
              # the piece settings.
              set space [string first " " $fenstr]
              set fen [string range $fenstr 0 $space]

              set space [string first " " [sc_pos fen]]
              set int [string range [sc_pos fen] 0 $space]

              if {$fen != $int} {
                ::utils::sound::PlayMove sound_end
                logEngine "  info Wrong Position! $int (scid) != $fen (external)"
              } else {
                logEngine "  info Board and internal position match."
              }
              # Generate a board position out of the FEN
              # RNBQKBNRPPPP.PPP............P................n..pppppppprnbqkb.r w
              # Something is in reverse here:
              ###---### set extpos $fen
              ###---### regsub -all {8} $extpos "........" extpos
              ###---### regsub -all {7} $extpos "......." extpos
              ###---### regsub -all {6} $extpos "......" extpos
              ###---### regsub -all {5} $extpos "....." extpos
              ###---### regsub -all {4} $extpos "...." extpos
              ###---### regsub -all {3} $extpos "..." extpos
              ###---### regsub -all {2} $extpos ".." extpos
              ###---### regsub -all {1} $extpos "." extpos
              ###---### regsub -all {/} $extpos "" extpos
              ###---### puts stderr [sc_pos board]
              ###---### puts stderr [string reverse "$extpos"]
              ###---### set extpos "$extpos w"
              ###---### ::board::update .inputengineconsole.bd "$extpos w"
            }
          } \
          {moving piece: [A-Z] *} {
            ::inputengine::setPieceImage $::board::letterToPiece([string range $event 14 end])80
          }\
          {moving piece: [a-z] *} {
            ::inputengine::setPieceImage $::board::letterToPiece([string range $event 14 end])80
          }\
          "!new game!" {
            ::inputengine::newgame
            .inputengineconsole.bPiece configure -background blue
            .inputengineconsole.bMove  configure -background blue -text "OK"
            ::inputengine::setPieceImage wk80
          } \
          "!move now!" {
            logEngine "< info $event"
          } \
          "!end game 1-0!" {
            logEngine "< info $event"
            ::inputengine::endgame "1-0"
            .inputengineconsole.bPiece configure -background white
            .inputengineconsole.bMove  configure -background white -text "1-0"
            ::inputengine::setPieceImage wk80
          } \
          "!end game 0-1!" {
            logEngine "< info $event"
            ::inputengine::endgame "0-1"
            .inputengineconsole.bPiece configure -background gray
            .inputengineconsole.bMove  configure -background gray -text "0-1"
            ::inputengine::setPieceImage bk80
          } \
          "!end game 1/2-1/2!" {
            logEngine "< info $event"
            ::inputengine::endgame "1/2-1/2"
            .inputengineconsole.bPiece configure -background black
            .inputengineconsole.bMove  configure -background white -text "1/2-1/2"
            ::inputengine::setPieceImage $::board::letterToPiece(.)80
          } \
          "!enter setup mode!" {
            .inputengineconsole.bPiece configure -background yellow
            .inputengineconsole.bMove  configure -background yellow -text "Setup"
            ::inputengine::setPieceImage wk80
            logEngine "< info $event"
          } \
          "!end setup mode!" {
            logEngine "< info $event"
            ::inputengine::synchronise
            .inputengineconsole.bPiece configure -background yellow
            .inputengineconsole.bMove  configure -background yellow -text "OK"
            ::inputengine::setPieceImage bq80
          } \
          "!white to move!" {
            set ::inputengine::toMove "White"
            .inputengineconsole.wClock configure -background white
            .inputengineconsole.bClock configure -background gray -foreground black

            if {$::inputengine::StoreClock == 1} {
               if { ($::inputengine::oldWhiteClock != $::inputengine::NoClockTime) && \
                    ($::inputengine::WhiteClock    != $::inputengine::NoClockTime) } {
                  set wHrs [expr $::inputengine::WhiteClock / 60 / 60]
                  set wMin [expr ($::inputengine::WhiteClock - $wHrs*60*60) / 60 ]
                  set wSec [expr ($::inputengine::WhiteClock - $wHrs*60*60 - $wMin * 60) ]
                  set timediff [expr $::inputengine::oldWhiteClock - $::inputengine::WhiteClock]
                  set ::inputengine::oldWhiteClock $::inputengine::WhiteClock
                  sc_pos setComment "\[%ct $wHrs:$wMin:$wSec\] \[%emt $timediff\]"
               }
            }
          } \
          "!black to move!" {
            set ::inputengine::toMove "Black"
            .inputengineconsole.wClock configure -background gray
            .inputengineconsole.bClock configure -background black -foreground white

            if {$::inputengine::StoreClock == 1} {
               if { ($::inputengine::oldBlackClock != $::inputengine::NoClockTime) && \
                    ($::inputengine::BlackClock    != $::inputengine::NoClockTime) } {
                  set bHrs [expr $::inputengine::BlackClock / 60 / 60]
                  set bMin [expr ($::inputengine::BlackClock - $bHrs*60*60) / 60 ]
                  set bSec [expr ($::inputengine::BlackClock - $bHrs*60*60 - $bMin * 60) ]
                  set timediff [expr $::inputengine::oldBlackClock - $::inputengine::BlackClock]
                  set ::inputengine::oldBlackClock $::inputengine::BlackClock
                  sc_pos setComment "\[%ct $bHrs:$bMin:$bSec\] \[%emt $timediff\]"
               }
            }
          } \
          "No Clock detected" {
             set ::inputengine::WhiteClock $::inputengine::NoClockTime
             set ::inputengine::BlackClock $::inputengine::NoClockTime
             .inputengineconsole.wClock configure -text $::inputengine::WhiteClock
             .inputengineconsole.bClock configure -text $::inputengine::BlackClock
          } \
          "Time White:" {
            if { ($::inputengine::oldWhiteClock == $::inputengine::NoClockTime) } {
               set ::inputengine::oldWhiteClock $::inputengine::WhiteClock
            }
            # Get the time in seconds
            regsub -all {[A-Za-z:# ]} $event "" ::inputengine::WhiteClock

            # calculate a sensible format
            set wHrs [expr $::inputengine::WhiteClock / 60 / 60]
            set wMin [expr ($::inputengine::WhiteClock - $wHrs*60*60) / 60 ]
            set wSec [expr ($::inputengine::WhiteClock - $wHrs*60*60 - $wMin * 60) ]

            if {$wHrs > 0} {
               .inputengineconsole.wClock configure -text "$wHrs:$wMin:$wSec (EXT)"
            } else {
               .inputengineconsole.wClock configure -text "$wMin:$wSec (EXT)"
            }

            ###---### Is this enough to set game clocks for all possible occurences?
            catch { ::gameclock::setSec 1 [expr -1*$::inputengine::WhiteClock] }
          } \
          "Time Black:" {
            if { ($::inputengine::oldBlackClock == $::inputengine::NoClockTime) } {
	      set ::inputengine::oldBlackClock $::inputengine::BlackClock
            }
            regsub -all {[A-Za-z:# ]} $event "" ::inputengine::BlackClock

            set bHrs [expr $::inputengine::BlackClock / 60 / 60]
            set bMin [expr ($::inputengine::BlackClock - $bHrs*60*60) / 60 ]
            set bSec [expr ($::inputengine::BlackClock - $bHrs*60*60 - $bMin * 60) ]

            if {$bHrs > 0} {
               .inputengineconsole.bClock configure -text "$bHrs:$bMin:$bSec (EXT)"
            } else {
               .inputengineconsole.bClock configure -text "$bMin:$bSec (EXT)"
            }

            ###---### Is this enough to set game clocks for all possible occurences?
            catch { ::gameclock::setSec 2 [expr -1*$::inputengine::BlackClock] }
          } \
          "Wrong move performed:" {
             # This event can only be used if there is a possiblity to
             # send the last move to the input engine for it ot cross
             # check. This however is not easy in Scid, therefore
             # compare FEN.
             #
             # ::utils::sound::PlayMove sound_end
             # logEngine "< info $event"
          } \
          "DGT Projects - This DGT board" {
            set ::inputengine::connectimg tb_eng_dgt
            set txt [string range $event 7 end]
            ## ::utils::tooltip::Set .main.button.exthardware "$::inputengine::port:\n$txt"
          } \
        } \
        default  {
          logEngine "< $line"
        }
        # Should better show current wooden board position? Return value of 
        # sc_pos board is
        # RNBQKBNRPPPP.PPP............P................n..pppppppprnbqkb.r w
        ::board::update .inputengineconsole.bd [sc_pos board]
  }

  proc setPieceImage {image} {
    if {$::macOS} {
      # Mac wish 8.5 does not like transparent images on some widgets (in this
      # case, .inputengineconsole.bPiece is a button) so remove transparency.
      # NB Mac buttons also ignore '-background', but we have not fixed this
      catch {image delete bPieceImage}
      image create photo bPieceImage -format png -data [$image data -format png -background white]
      .inputengineconsole.bPiece configure -image bPieceImage
    } else {
      .inputengineconsole.bPiece configure -image $image
    }
  }

}

###
### End of file: inputengine.tcl
###
