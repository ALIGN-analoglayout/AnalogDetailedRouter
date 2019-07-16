#! /usr/bin/tclsh

# custom option
#/usr/intel/bin/tclsh8.5

# another variant to run is
# > tclsh /path/to/this/script

namespace eval ::cln {

    set ::_xUnit 1000
    set ::_yUnit 400

    set ::_numXUnits 8
    set ::_numYUnits 16

    set ::_grW [expr {$::_numXUnits * $::_xUnit}]
    set ::_grH [expr {$::_numYUnits * $::_yUnit}]

    set ::_m1Period 1000
    set ::_m1Offset 500
    set ::_m1Width  700
    set ::_m1MinEte 300
    
    set ::_wiresPerGR 7
    
    set ::_gid 0

    set ::_abc [list a b c d e f g h i j k l m n o p q r s t u v w x y z]
    
    #
    proc generate_test_case {netlistfilename grfilename mtifilename archfilename patternfilename runfilename} {

        set ::_numGRCols [expr {$::_busWidth * 2 + 6}]
        set ::_numGRRows [expr {$::_busWidth * 2 + 0}]
        set ::_grOffset  [expr {$::_numGRRows - $::_busWidth - 2}]

        set ::_cellxl 0
        set ::_cellyl 0
        set ::_cellxh [expr {$::_numGRCols * $::_grW}]
        set ::_cellyh [expr {$::_numGRRows * $::_grH}]

        set ofs0 [open $netlistfilename "w"]
        set ofs1 [open $grfilename      "w"]
        set ofs2 [open $mtifilename     "w"]
        set ofs3 [open $archfilename    "w"]
        set ofs4 [open $patternfilename "w"]
        set ofs5 [open $runfilename     "w"]

        if {$ofs0 == ""} { error "Could not open file for writing: $netlistfilename" }
        if {$ofs1 == ""} { error "Could not open file for writing: $grfilename" }
        if {$ofs2 == ""} { error "Could not open file for writing: $mtifilename" }
        if {$ofs3 == ""} { error "Could not open file for writing: $archfilename" }
        if {$ofs4 == ""} { error "Could not open file for writing: $patternfilename" }
        if {$ofs5 == ""} { error "Could not open file for writing: $runfilename" }

        # runfile
        write_run_file_ $ofs5 $netlistfilename $grfilename $mtifilename $archfilename $patternfilename

        # patterns
        write_pattern_file_ $ofs4

        # arch
        puts $ofs3 "Option name=gr_region_width_in_poly_pitches value=${::_numXUnits}"
        puts $ofs3 "Option name=gr_region_height_in_diff_pitches value=${::_numYUnits}"
        
        # mti
        for {set i 0} {$i <= 6} {incr i} {

            set layerindex $i
            set mtindex    0
            set offset     0
            
            # custom
            if {$layerindex == 1} { set offset $::_m1Offset }
            if {$layerindex == 2} { set mtindex 1 }
            
            set mtname "m${layerindex}_template_0${mtindex}"

            puts $ofs2 "MetalTemplateInstance template=${mtname} pgdoffset_abs=${offset} region=${::_cellxl}:${::_cellyl}:${::_cellxh}:${::_cellyh}"
        }

        # netlist
        puts $ofs0 "Cell name=circuit01 bbox=${::_cellxl}:${::_cellyl}:${::_cellxh}:${::_cellyh}"
        puts $ofs0 ""

        set maxnetid ""

        set westx 1
        set eastx [expr {$::_numGRCols - 2}]

        set westyl 1
        set westyh [expr {$westyl + $::_busWidth - 1}]

        set eastyl [expr {$westyl + $::_grOffset}]
        set eastyh [expr {$eastyl + $::_busWidth - 1}]
        
        for {set mode 0} {$mode <= 1} {incr mode} {

            set netid 0

            if {$mode == 0} {
                set minx $westx
                set maxx $westx
                set miny $westyl
                set maxy $westyh
                
            } elseif {$mode == 1} {
                set minx $eastx
                set maxx $eastx
                set miny $eastyl
                set maxy $eastyh
                
            } else {
                error ""
            }

            # netlist
            for {set w 0} {$w < $::_wiresPerGR} {incr w} {

                for {set x $minx} {$x <= $maxx} {incr x} {

                    for {set y $miny} {$y <= $maxy} {incr y} {

                        #set wireindex [expr {1 + $w * 2}]
                        set wireindex [expr {0 + $w}]

                        write_wire_ $ofs0 $ofs1 $x $y $wireindex $netid
                        incr netid
                    }
                }
            }

            set maxnetid [expr {$netid - 1}]
        }

        set horlayername "metal2"
        set verlayername "metal3"

        set bendx  [expr {$eastx - 2}]
        
        for {set netid 0} {$netid <= $maxnetid} {incr netid} {

            set netname [netid_2_netname_ $netid]
            
            set yoffset [expr {$netid % $::_busWidth}]
            set xoffset $yoffset

            set x2 [expr {$bendx - $xoffset}]

            set xl $westx
            set xh $x2
            set yl [expr {$westyl + $yoffset}]
            set yh $yl
            if {$xl >= $xh} { error "GR is too short" }
            puts $ofs1 "GlobalRouting net=${netname} routes=($xl,$yl,$xh,$yh,$horlayername)"

            set xl $xh
            set xh $xl
            set yh [expr {$yl + $::_grOffset}]
            if {$yl >= $yh} { error "GR is too short" }
            puts $ofs1 "GlobalRouting net=${netname} routes=($xl,$yl,$xh,$yh,$verlayername)"

            set xh $eastx
            set yl $yh
            set yh $yl
            if {$xl >= $xh} { error "GR is too short" }
            puts $ofs1 "GlobalRouting net=${netname} routes=($xl,$yl,$xh,$yh,$horlayername)"
        }

        close $ofs0
        close $ofs1
        close $ofs2
        close $ofs3
        close $ofs4
        close $ofs5

        puts "Writted $netlistfilename"
        puts "Writted $grfilename"
        puts "Writted $mtifilename"
        puts "Writted $archfilename"
        puts "Writted $patternfilename"
        puts "Writted $runfilename"
        
    }
    # end of generate_test_case

    #
    proc write_pattern_file_ {ofs} {

        puts $ofs "# disabled for a while; remove _ before Pattern to enable the pattern for the router"
        puts $ofs "_Pattern name=via1_diagonal_2x2 shortname=V1_00_2x2 comment=\"Corner-to-corner spacing 2x2\" \{"
        puts $ofs "  Node type=AND \{"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=0:0:660:460"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=1000:800:1660:1260"
        puts $ofs "  \}"
        puts $ofs "\}"
        puts $ofs ""

        puts $ofs "# disabled for a while; remove _ before Pattern to enable the pattern for the router"
        puts $ofs "_Pattern name=via1_diagonal_3x2 shortname=V1_00_3x2 comment=\"Corner-to-corner spacing 3x2\" \{"
        puts $ofs "  Node type=AND \{"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=0:0:660:460"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=2000:800:2660:1260"
        puts $ofs "  \}"
        puts $ofs "\}"
        puts $ofs ""

        puts $ofs "# disabled for a while; remove _ before Pattern to enable the pattern for the router"
        puts $ofs "_Pattern name=via1_diagonal_2x3 shortname=V1_00_2x3 comment=\"Corner-to-corner spacing 2x3\" \{"
        puts $ofs "  Node type=AND \{"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=0:0:660:460"
        puts $ofs "    Node type=leaf pr=mbp layer=via1 rect=1000:1600:1660:2060"
        puts $ofs "  \}"
        puts $ofs "\}"
        puts $ofs ""
    }
    # end of write_pattern_file_

    #
    proc write_run_file_ {ofs netlistfilename grfilename mtifilename archfilename patternfilename} {

        puts $ofs "# File auto-generated"
        puts $ofs "# user: $::env(USER)"
        puts $ofs ""

        puts $ofs "# fixed"
        puts $ofs "Option name=layer_file          value=COLLATERAL/layers.txt"
        puts $ofs "Option name=metal_template_file value=COLLATERAL/metal_templates.txt"
        puts $ofs "Option name=generator_file      value=COLLATERAL/via_generators.txt"
        puts $ofs "Option name=option_file         value=COLLATERAL/design_rules.txt"
        puts $ofs ""
        puts $ofs "# custom"
        puts $ofs "Option name=input_file          value=${netlistfilename}"
        puts $ofs "Option name=global_routing_file value=${grfilename}"
        puts $ofs "Option name=option_file         value=${mtifilename}"
        puts $ofs "Option name=arch_file           value=${archfilename}"
        puts $ofs "Option name=pattern_file        value=${patternfilename}"
        puts $ofs ""
        puts $ofs "# run options"
        puts $ofs "Option name=route       value=1"
        puts $ofs "Option name=solver_type value=glucose"
        puts $ofs "Option name=allow_opens value=1"
        puts $ofs ""
        puts $ofs "# flow options"
        puts $ofs "Option name=auto_fix_global_routing value=0"
        puts $ofs ""
        puts $ofs "# control nets to route / not to route"
        puts $ofs "#Option name=nets_to_route value=a*"
        puts $ofs "Option name=nets_not_to_route value=KOR"
        puts $ofs ""
        puts $ofs "# options for visualization"
        puts $ofs "Option name=create_fake_global_routes            value=0"
        puts $ofs "Option name=create_fake_connected_entities       value=0"
        puts $ofs "Option name=create_fake_ties                     value=0"
        puts $ofs "Option name=create_fake_metal_template_instances value=0"
        puts $ofs "Option name=create_fake_sat_wires                value=0"
        puts $ofs "Option name=dump_global_regions                  value=1"
        puts $ofs "Option name=dump_global_regions_with_coordinates value=0"

    }
    # end of write_run_file_

    #
    proc netid_2_netname_ {netid} {

        set base $::_busWidth

        if {$netid < 0} { error "" }

        set index    [expr {$netid % $base}]
        set busindex [expr {($netid - $index) / $base}]

        if {$busindex < 0 || $busindex >= [llength $::_abc]} { error "busindex is too large" }

        set netbasename [lindex $::_abc $busindex]
        
        set netname "${netbasename}\[${index}\]"
        
        return $netname
    }
    # end of netid_2_netname_

    #
    proc write_wire_ {ofs0 ofs1 x y track netid} {

        set period     $::_m1Period
        set offset     $::_m1Offset
        set width      $::_m1Width
        set minete     $::_m1MinEte
        set layername "metal1"
        set netname    [netid_2_netname_ $netid]
        
        set grxl [expr {$::_cellxl + $x * $::_grW}]
        set grxh [expr {$grxl + $::_grW}]

        set gryl [expr {$::_cellyl + $y * $::_grH}]
        set gryh [expr {$gryl + $::_grH}]

        set a [expr {$grxl - $offset}]
        set b [expr {$a % $period}]
        set numtracks [expr {($a - $b) / $period}]

        set xc0 [expr {$offset + $numtracks * $period}]
        if {$xc0 < $grxl} {
            incr numtracks
        }
        incr numtracks $track
        
        set xc [expr {$offset + $numtracks * $period}]
        
        set xl [expr {$xc - $width / 2}]
        set xh [expr {$xc + $width / 2}]

        set yl [expr {$gryl + $minete * 4}]
        set yh [expr {$gryh - $minete * 4}]
        
        # netlist file
        puts $ofs0 "Wire net=$netname layer=$layername rect=$xl:$yl:$xh:$yh gid=$::_gid"

        # gr file
        puts $ofs1 "ConnectedEntity terms=$::_gid"
        
        incr ::_gid
    }
    # end of write_wire_

}
# end of of namespace ::cln

set i 0
set ::_busWidth   [lindex $argv $i] ; incr i
set ::_wiresPerGR [lindex $argv $i] ; incr i
set prefix        [lindex $argv $i] ; incr i

if {$::_busWidth == ""} {
    set ::_busWidth 8
}

if {$::_wiresPerGR == ""} {
    set ::_wiresPerGR 3
}

if {$::_busWidth <= 0} {
    error "illegal bus width: $::_busWidth"
}

if {$::_wiresPerGR <= 0} {
    error "illegal number of wires per GR: $::_wiresPerGR"
}

if {$prefix == ""} {
    set prefix test_b${::_busWidth}_w${::_wiresPerGR}
}

set outdir CUSTOM
file mkdir $outdir

set netlistfilename ${outdir}/${prefix}_netlist.txt
set grfilename      ${outdir}/${prefix}_gr.txt
set mtifilename     ${outdir}/${prefix}_mti.txt
set archfilename    ${outdir}/${prefix}_arch.txt
set patternfilename ${outdir}/${prefix}_patterns.txt
set runfilename     ${prefix}_run.txt

::cln::generate_test_case $netlistfilename $grfilename $mtifilename $archfilename $patternfilename $runfilename
