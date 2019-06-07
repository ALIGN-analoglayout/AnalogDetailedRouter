#set ::env(AMSRDIR) /nfs/sc/disks/scl.work.01/ppt/nvryzhen/cellGen/fls/analog
#set ::env(AMSRDIR) /nfs/sc/disks/scl.work.01/ppt/nvryzhen/releases/amsr

#if {[array names ::env AMSRDIR] == ""} {
#    error "Environment variable AMSRDIR is not set."
#}

set filenames [list]
#lappend filenames $::env(AMSRDIR)/tcl/token.tcl

foreach filename $filenames {
    if {![file exists $filename]} {
        error "File does not exist: $filename"
    }
    puts "source $filename"
    source $filename
}

namespace eval ::cln {
 
    # some default values
    set ::_scale            1
    set ::_rowHeight        ""
    set ::_rowHeightOutput  ""
    set ::_diffPitch        ""
    set ::_polyPitch        ""
    set ::_polyPitchOutput  ""
    set ::_commentWires     0
    set ::_skipDummyDevices 0
    set ::_tech             p1274.1
    set ::_carFileName      /nfs/sc/proj/dt/cadbackend08/em64t_SLES10/techfiles/alltechs/${::_tech}.car
    set ::_flatKorNetName  "ALS_KOR_DO_NOT_ROUTE" ; # used to mark all internal nets as KOR polygons
    set ::_divaPaths       ""
    set ::_parsedNesting   ""
    set ::_errFile         ""
    set ::_modelMap        ""
    set ::_cellNameMap     ""
    
    #set ::_tech p1273.1
    #set ::_carFileName /nfs/sc/disks/scl.work.01/ppt/nvryzhen/cellGen/amsrundir/car/p1273.1.car

    proc print_global_vars {} {

        puts "::_scale            $::_scale "
        puts "::_rowHeight        $::_rowHeight"
        puts "::_rowHeightOutput  $::_rowHeightOutput"
        puts "::_diffPitch        $::_diffPitch"
        puts "::_polyPitch        $::_polyPitch"
        puts "::_polyPitchOutput  $::_polyPitchOutput"
        puts "::_commentWires     $::_commentWires"
        puts "::_skipDummyDevices $::_skipDummyDevices"
        puts "::_tech             $::_tech"
        puts "::_carFileName      $::_carFileName"
        puts "::_flatKorNetName   $::_flatKorNetName"
        puts "::_divaPaths        $::_divaPaths"
        puts "::_modelMap         $::_modelMap"
        puts "::_cellNameMap      $::_cellNameMap"
    }
    # end of print_global_vars
    
    #
    proc geometry_get_xl     {geometry} { return [lindex $geometry 0 ] }
    proc geometry_get_yl     {geometry} { return [lindex $geometry 1 ] }
    proc geometry_get_xh     {geometry} { return [lindex $geometry 2 ] }
    proc geometry_get_yh     {geometry} { return [lindex $geometry 3 ] }
    proc geometry_get_layer  {geometry} { return [lindex $geometry 4 ] }
    proc geometry_get_net    {geometry} { return [lindex $geometry 5 ] }
    proc geometry_get_ported {geometry} { return [lindex $geometry 6 ] }

    #
    proc geometry_set_xl     {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 0 $v }
    proc geometry_set_yl     {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 1 $v }
    proc geometry_set_xh     {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 2 $v }
    proc geometry_set_yh     {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 3 $v }
    proc geometry_set_layer  {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 4 $v }
    proc geometry_set_net    {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 5 $v }
    proc geometry_set_ported {geometry_ref v} { upvar $geometry_ref geometry; lset geometry 6 $v }

    #
    proc parse_cell_nestings {cell divapaths} {

        set divapaths [split $divapaths ":"]

        set cellname [cell_get_name $cell]
        set cellname [string range $cellname 0 16]
        
        foreach divapath $divapaths {

            set divapath [string trim $divapath]
            if {$divapath == ""} { continue }

            if {![file exists $divapath]} { error "File does not exist: $divapath" }
            
            set divafile [glob -nocomplain -directory $divapath $cellname*] 
            
            if {[llength $divafile] == 0} {
                puts "Nesting specs not found for $cellname in $divapath. No nesting."
                continue
            }

            set fp [open $divafile "r"]
            if {$fp == ""} {
                error "Could not open file for reading: $divafile"
            }

            set nestings [list]

            while {[gets $fp line] >= 0} {
                set line [string trim $line]
                if {$line == ""} { continue }
                set devicename [lindex $line 0]
                if {[string index $devicename 0] != "m"} { continue }
                set devicename [string range $devicename 1 end]
                set nestingstr [lindex $line 5]
                if {[string is double $nestingstr]} {
                    set nesting [expr {double($nestingstr) / 100}]
                    lappend nestings [list $devicename $nesting]
                } else {
                    set errmsg "Could not parse nesting spec for device line '$line', expected a number and got '$nestingstr'"
                    if {$::_errFile != ""} { puts $::_errFile $errmsg } else { puts $errmsg }
                }
            }

            close $fp

            return $nestings
        }

        return ""
    }
    # end of parse_cell_nestings

    #
    proc get_cell_nestings {cell} {

        set targetcellname [cell_get_name $cell]
        
        for {set i [expr {[llength $::_parsedNesting] - 1}]} {$i >= 0} {incr i -1} {

            set iter0 [lindex $::_parsedNesting $i]
            
            set j 0
            set cellname [lindex $iter0 $j] ; incr j
            set nestings [lindex $iter0 $j] ; incr j
            
            if {$cellname != $targetcellname} { continue }

            return $nestings
        }

        set nestings [parse_cell_nestings $cell $::_divaPaths]
        
        lappend ::_parsedNesting [list $targetcellname $nestings]
        
        return $nestings
    }
    # end of get_cell_nestings

    #
    proc get_device_nesting {cell device} {

        set targetdevicename [igen_get_name $device]
        
        set nestings [get_cell_nestings $cell]
        
        foreach iter1 $nestings {

            #puts $iter1
            
            set j 0
            set devicename [lindex $iter1 $j] ; incr j
            set nesting    [lindex $iter1 $j] ; incr j
            
            if {$devicename == $targetdevicename} { return $nesting }
        }
        
        return ""
    }
    # end of get_device_nesting

    #
    proc convert_lnfs_to_lgfs {groupfilename outdir} {

        set stopfile "~/stopfile"

        if {$::_rowHeight == ""} {
            error "::_rowHeight is not set"
            return
        }

        if {$groupfilename == ""} {
            set groupfilename "/nfs/sc/disks/adlt_sc_02/nvryzhen/rundirs/i00rundir/group/g0m_5_cells.group"
        }

        file mkdir $outdir
        
        if {![file exists $outdir]} {
            error "File does not exist: $outdir"
        }
        if {![file isdirectory $outdir]} {
            error "Not a directory: $outdir" 
        }

        set ifs [open $groupfilename "r"]

        set lnfnames [list]

        while {[gets $ifs line] >= 0} {

            set line [string trim $line]
            if {$line == ""} { continue }

            if {[string range $line 0 0] == "#"} { continue }

            lappend lnfnames $line
        }

        close $ifs

        
        set ::_errFile     ""
        set errfilename ""
        set errfilename "[pwd]/error.log"

        if {$errfilename != ""} {
            set ::_errFile [open $errfilename "w"]
            if {$::_errFile == ""} {
                error "Could not open $errfilename"
            }
        }

        foreach lnfname $lnfnames {
            
            if {[file exists $stopfile]} {
                puts "Aborted. A stop file exists: $stopfile"
                break
            }
            
            set ret [convert_lnf_to_lgf $lnfname $outdir]
            if {$ret != 0} {
                puts "Found an error."
                #break
            }
        }

        if {$::_errFile != ""} {
            close $::_errFile
            puts "Written $errfilename"
        }
    }
    # end of convert_lnfs_to_lgfs

    #
    proc convert_lnf_to_lgf {lnfname outdir} {

        set outfilename [get_architecture_specific_cell_name $lnfname]
        set outfilepath ${outdir}/${outfilename}.lgf

        if {[file exists $outfilepath]} {
            #set errmsg "File already exists: $outfilepath"
            #if {$::_errFile != ""} { puts $::_errFile $errmsg } else { puts $errmsg }
            return 0
        }

        ::cadworks::open -cellname $lnfname -viewname "lnf" -checkout 0

        set cell [get_active_cell]
        if {$cell == ""} {
            set errmsg "Could not open $lnfname"
            if {$::_errFile != ""} { puts $::_errFile $errmsg } else { puts $errmsg }
            return 1
        }

        set cellname [cell_get_name $cell]

        if {$cellname != $lnfname} {
            set errmsg "Could not open $lnfname"
            if {$::_errFile != ""} { puts $::_errFile $errmsg } else { puts $errmsg }
            return 1
        }

        ::cln::dump_flat_layout $outfilepath

        ::cadworks::discard -cellname $cellname -noask -nowindow
        
        return 0
    }
    # end of convert_lnf_to_lgf

    #
    proc convert_active_cell_to_lgf {outdir} {

        set cell [get_active_cell]
        if {$cell == ""} {
            error "No cell opened"
            return ""
        }

        file mkdir $outdir

        set cellname [cell_get_name $cell]
        
        set outfilepath [file normalize "${outdir}/${cellname}.lgf"]
        
        ::cln::dump_flat_layout $outfilepath
        
        return ${outfilepath}
    }
    # end of convert_active_cell_to_lgf
    
    #
    proc scale_bbox {str} {
        
        set values [split $str ":"]
        
        set ret [list]
        
        foreach value $values {
            lappend ret [expr {$value / $::_scale}]
        }
        
        return [join $ret ":"]
    }
    # end of scale_bbox

    #
    proc get_architecture_specific_bbox {str} {

        if {$::_polyPitch == ""} { error "get_architecture_specific_bbox: ::_polyPitch is not set" }
        if {$::_polyPitchOutput == ""} { error "get_architecture_specific_bbox: ::_polyPitchOutput is not set" }
        if {$::_rowHeight == ""} { error "get_architecture_specific_bbox: ::_rowHeight is not set" }

        set values [split $str ":"]
        if {[llength $values] < 4} { error "unexpected bbox: $str" }

        set i 0
        set xl [lindex $values $i] ; incr i
        set yl [lindex $values $i] ; incr i
        set xh [lindex $values $i] ; incr i
        set yh [lindex $values $i] ; incr i

        if {$xl == ""} { error "xl is not set" }
        if {$yl == ""} { error "yl is not set" }
        if {$xh == ""} { error "xh is not set" }
        if {$yh == ""} { error "yh is not set" }

        if {$::_polyPitchOutput != ""} {
            set xl [expr {($xl / $::_polyPitch) * $::_polyPitchOutput}]
            set xh [expr {($xh / $::_polyPitch) * $::_polyPitchOutput}]
        }

        if {$::_rowHeightOutput != ""} {
            set yl [expr {($yl / $::_rowHeight) * $::_rowHeightOutput}]
            set yh [expr {($yh / $::_rowHeight) * $::_rowHeightOutput}]
        }

        set ret [list $xl $yl $xh $yh]
        
        return [join $ret ":"]
    }
    # end of get_architecture_specific_bbox

    #
    proc get_architecture_specific_cell_name {name} {

        foreach iter $::_cellNameMap {

            set length [string length $name]

            set i 0
            set index [lindex $iter $i] ; incr i
            set str   [lindex $iter $i] ; incr i

            if {$index < 0} { error "unexpected index" }
            if {$index >= $length} { error "unexpected index" }

            if {$index == 0} {
                set part0 ""
            } else {
                set part0 [string range $name 0 [expr {$index - 1}]]
            }
            set part1 $str
            set part2 [string range $name [expr {$index + [string length $str]}] end]

            #puts "iter:  $iter"
            #puts "name:  $name"
            #puts "part0: $part0"
            #puts "part1: $part1"
            #puts "part2: $part2"

            set name ${part0}${part1}${part2}

            if {[string length $name] != $length} { error "unexpected name conversion" }
        }
        
        return $name
    }
    # end of get_architecture_specific_cell_name

    proc geometry_create {x1 y1 x2 y2 layer net {ported 0}} {

        if {$x1 < $x2} {
            set xl $x1
            set xh $x2
        } else {
            set xl $x2
            set xh $x1
        }

        if {$y1 < $y2} {
            set yl $y1
            set yh $y2
        } else {
            set yl $y2
            set yh $y1
        }

        if {$::_scale != ""} {

            set xl [expr {$xl / $::_scale}]
            set yl [expr {$yl / $::_scale}]
            set xh [expr {$xh / $::_scale}]
            set yh [expr {$yh / $::_scale}]
        }
        
        return [list $xl $yl $xh $yh $layer $net $ported]
    }
    # end of geometry_create
    
    #
    proc geometry_dump {fileout geometry} {

        set xl     [geometry_get_xl     $geometry]
        set yl     [geometry_get_yl     $geometry]
        set xh     [geometry_get_xh     $geometry]
        set yh     [geometry_get_yh     $geometry]
        set layer  [geometry_get_layer  $geometry]
        set net    [geometry_get_net    $geometry]
        set ported [geometry_get_ported $geometry]

        set layer [split $layer ":"]
        set layer [lindex $layer [expr {[llength $layer] - 1}]]
        
        set bbox "${xl}:${yl}:${xh}:${yh}"

        set comment ""
        if {$::_commentWires} {
            set comment "#"
        }

        set str "${comment}Wire net=${net} layer=${layer} rect=${bbox}"

        if {$ported != "" && $ported} {
            append str " ported=${ported}"
        }
        
        myputs $fileout $str
    }
    # end of geometry_dump
    
    #
    proc parse_transfrom {tr} {
        
        if {$tr == ""} {
            error "parse_transfrom: no tr"
        }
        
        set tr [string map {: " "} $tr]
        set tr [string map {_ " "} $tr]
        
        set shiftx [lindex $tr 0]
        set shifty [lindex $tr 1]
        
        set rot [lindex $tr 3]
        set ref [lindex $tr 5]
        
        set parsedtransform [list $shiftx $shifty $rot $ref]
        
        return $parsedtransform
    }
    # end of parse_transfrom

    #
    proc transform_xy {x y tr} {

        if {$tr == ""} {
            return [list $x $y]
        }
        
        set parsedtransform [parse_transfrom $tr]
        
        set shiftx [lindex $parsedtransform 0]
        set shifty [lindex $parsedtransform 1]
        set rot    [lindex $parsedtransform 2]
        set ref    [lindex $parsedtransform 3]
        
        # ref&rot
        if     {$ref == "0"  && $rot == 0} {
            
        } elseif {$ref == "0"  && $rot == 180} {
            set x [expr {-1 * $x}]
            set y [expr {-1 * $y}]
        } elseif {$ref == "Y"  && $rot == 0} {
            set x [expr {-1 * $x}]
        } elseif {$ref == "X"  && $rot == 0} {
            set y [expr {-1 * $y}]
        } elseif {$ref == "XY" && $rot == 0} {
            error "transform_xy: forbidden rot/ref: $rot/$ref; identical to 0/0"
        } elseif {$ref == "Y"  && $rot == 180} {
            error "transform_xy: forbidden rot/ref: $rot/$ref; identical to X/0"
        } elseif {$ref == "X"  && $rot == 180} {
            error "transform_xy: forbidden rot/ref: $rot/$ref; identical to Y/0"
        } elseif {$ref == "XY" && $rot == 180} {
            error "transform_xy: forbidden rot/ref: $rot/$ref; identical to 0/180"
        } else {
            error "transform_xy: unexpected rot/ref: $rot/$ref"
        }
        
        # shift
        set x [expr {$x + $shiftx}]
        set y [expr {$y + $shifty}]
        
        return [list $x	$y]
    }
    # end of transform_xy

    proc transform_x {x tr} {
        
        set newx [lindex [transform_xy $x 0 $tr] 0]
        
        return $newx
    }
    # end of transform_x
    
    proc transform_y {y tr} {
        
        set newy [lindex [transform_xy 0 $y $tr] 1]
        
        return $newy
    }
    # end of transform_y
    
    #
    proc add_if_unique {value values_ref} {

        upvar $values_ref values

        if {[lsearch -exact $values $value] >= 0} { return 0 }

        lappend values $value

        return 1
    }
    # end of add_if_unique
    
    #
    proc geometry_is_redundant {geometry geometries_ref} {

        upvar $geometries_ref geometries

        set xl1    [geometry_get_xl    $geometry]
        set yl1    [geometry_get_yl    $geometry]
        set xh1    [geometry_get_xh    $geometry]
        set yh1    [geometry_get_yh    $geometry]
        set layer1 [geometry_get_layer $geometry]

        foreach geometry2 $geometries {

            set xl2    [geometry_get_xl    $geometry2]
            set yl2    [geometry_get_yl    $geometry2]
            set xh2    [geometry_get_xh    $geometry2]
            set yh2    [geometry_get_yh    $geometry2]
            set layer2 [geometry_get_layer $geometry2]

            if {$layer2 != $layer1} { continue }

            if {$xl1 >= $xl2 && $yl1 >= $yl2 && $xh1 <= $xh2 && $yh1 <= $yh2} { return 1 }
        }

        return 0
    }
    # end of geometry_is_redundant

    #
    proc myputs {fileout str} {

        if {$fileout != ""} {
            puts $fileout $str
        } else {
            puts $str
        }

    }
    # end of myputs

    #
    proc create_net_hier_name {name hierarchy} {

        return [create_hier_name $name $hierarchy "/" "%"]
    }
    # end of create_net_hier_name

    #
    proc create_cell_hier_name {name hierarchy} {

        return [create_hier_name $name $hierarchy "/" "/"]
    }
    # end of create_cell_hier_name

    #
    proc create_hier_name {name hierarchy delim1 delim2} {

        if {[llength $hierarchy] == 0} { return $name }

        set hiername ""

        foreach inst $hierarchy {
            set instname [inst_get_name $inst]
            if {$hiername != ""} {
                append hiername $delim1
            }
            append hiername "${instname}"
        }

        if {$name != ""} {
            append hiername $delim2
            append hiername $name
        }

        return $hiername
    }
    # end of create_hier_name
    
    #
    proc net_is_syn {net} {

        set synnetbasename "syn"

        set netname [net_get_name $net]

        if {[string first $synnetbasename $netname] == 0} { return 1 }

        return 0
    }
    # end of net_is_syn

    #
    proc net_is_top_level {net hierarchy} {

        if {[llength $hierarchy] == 0} { return 1 }

        set inst [lindex $hierarchy end]
        
        set ipins [inst_get_ipins $inst]

        set foundexternalnet ""

        foreach ipin $ipins {
            
            set ownerinst   [ipin_get_owner_inst   $ipin]
            set externalnet [ipin_get_external_net $ipin]
            set masterpin   [ipin_get_master_pin   $ipin]

            if {$ownerinst != $inst} { error "unexpected hierarchy" }

            set pinnet [pin_get_net $masterpin]

            if {$pinnet == $net} {
                set foundexternalnet $externalnet
                break
            }
        }

        # it's an internal net
        if {$foundexternalnet == ""} {    
            return 0
        }
        
        set hierarchy1 [lrange $hierarchy 0 end-1]
        
        return [net_is_top_level $foundexternalnet $hierarchy1]
    }
    # end of net_is_top_level

    # return
    proc net_get_top_level_name {net hierarchy} {

        # quick test
        if {0} {

            set ipins [net_get_ipins $net]

            foreach ipin $ipins {
                
                set inst        [ipin_get_owner_inst   $ipin]
                set externalnet [ipin_get_external_net $ipin]
                set masterpin   [ipin_get_master_pin   $ipin]

                if {$externalnet != $net} { error "unexpected ipin-to-net connection" }
                
                set imaster [inst_get_master_cell $inst]
            }
        }

        # found top-level net
        if {[llength $hierarchy] == 0} { 
        
            set netname [net_get_name $net]
            
            return $netname
        }
        
        set inst [lindex $hierarchy end]
        
        set ipins [inst_get_ipins $inst]

        set foundexternalnet ""

        foreach ipin $ipins {
            
            set ownerinst   [ipin_get_owner_inst   $ipin]
            set externalnet [ipin_get_external_net $ipin]
            set masterpin   [ipin_get_master_pin   $ipin]

            if {$ownerinst != $inst} { error "unexpected hierarchy" }

            set pinnet [pin_get_net $masterpin]

            if {$pinnet == $net} {
                set foundexternalnet $externalnet
                break
            }
        }

        # it's an internal net
        if {$foundexternalnet == ""} {

            if {0} {
                set netname [create_net_hier_name [net_get_name $net] $hierarchy]
            } else {
                set netname $::_flatKorNetName
            }
            
            return $netname
        }
        
        set hierarchy1 [lrange $hierarchy 0 end-1]
        
        return [net_get_top_level_name $foundexternalnet $hierarchy1]
    }
    # end of net_get_top_level_name
    
    #
    proc polygon_apply_transformation {polygon hierarchy} {

        set layer [polygon_get_layer $polygon]

        set numedges [polygon_get_num_edges $polygon]
        if {$numedges != 4} {
            puts "Unexpected number of edges: $numedges; layer=$layer. Skipped."
            return ""
        }
        
        # UDM calculation
        set bbox [bbox_from_xy 0 0 0 0]
        polygon_get_bounding_box $polygon $bbox

        set minx [bbox_get_xl $bbox]
        set miny [bbox_get_yl $bbox]
        set maxx [bbox_get_xh $bbox]
        set maxy [bbox_get_yh $bbox]

        for {set i [expr {[llength $hierarchy] - 1}]} {$i >= 0} {incr i -1} {
            
            set inst [lindex $hierarchy $i]
            set transform [inst_get_transform $inst]

            set minx [transform_x $minx $transform]
            set maxx [transform_x $maxx $transform]
            set miny [transform_y $miny $transform]
            set maxy [transform_y $maxy $transform]
        }
        
        if {$minx > $maxx} {
            set tmpx $minx
            set minx $maxx
            set maxx $tmpx
        }
        
        if {$miny > $maxy} {
            set tmpy $miny
            set miny $maxy
            set maxy $tmpy
        }   
        
        set netname ""
        
        return [geometry_create $minx $miny $maxx $maxy $layer $netname]
    }
    # end of polygon_apply_transformation
    
    #
    proc cell_dump_flat_layout {fileout cell hierarchy optionDumpTopLevelNetsOnly optionSkipSynNets optionTheOnlyNetToDump} {
        
        set cellname        [cell_get_name          $cell]
        set devices         [cell_get_devices       $cell]
        set pins            [cell_get_pins          $cell]
        set nets            [cell_get_nets          $cell]
        set insts           [cell_get_child_insts   $cell]
        set celllayoutobjs  [cell_get_layout_objs   $cell]
        set cellgeoobjs     [cell_get_geo_objs      $cell]
        set cellgeopolygons [cell_get_geo_polygons  $cell 1]
        set cellpolygonsRaw [udm_utils_get_polygons $cell]

        set hiercellname [create_cell_hier_name "" $hierarchy]

        set cellpolygons [list]

        foreach iter1 $cellpolygonsRaw {
            foreach iter2 $iter1 {
                lappend cellpolygons $iter2
            }
        }
        
        set dumpDevicePolygons  0
        set dumpNetLayoutObjs   0
        set dumpNetGeoPolygons  1 ; # polygons may have net; for example, nwell
        set dumpCellLayoutObjs  1 ; # wire and vias of nets
        set dumpCellGeoPolygons 0 ; # polygons may have nets
        set dumpCellGeoObjs     0 ; # polygons without net; for example, ID polygons, slie, slia
        set dumpCellPolygons    0 ; # polygons without net

        if {$optionDumpTopLevelNetsOnly} {
            set dumpDevicePolygons  0
            set dumpNetLayoutObjs   1
            set dumpNetGeoPolygons  0
            set dumpCellLayoutObjs  0
            set dumpCellGeoPolygons 0
            set dumpCellGeoObjs     0
            set dumpCellPolygons    0
        }
        
        set polygongeometriesAll [list]
        
        if {[llength $hierarchy] == 0} {
            
            set boundary [cell_get_boundary $cell]
            set bbox [gig_figure_get_bbox $boundary]
            set bbox [scale_bbox $bbox]
            set bbox [get_architecture_specific_bbox $bbox]
            set cellname [get_architecture_specific_cell_name $cellname]
            
            myputs ${fileout} "Cell ${cellname} bbox=${bbox}"
            #myputs ${fileout} "Cell m16_test bbox=${bbox}"
            myputs ${fileout} ""

            # LGF info
            if {1} {                
                foreach pin $pins {
                    set pinname      [pin_get_name $pin]
                    set pindirection [pin_get_direction $pin]
                    if {$pindirection != "In" && $pindirection != "Out"} { continue }
                    #myputs ${fileout} "  Nets type=${pindirection} \[ $pinname \]"
                    myputs ${fileout} "  Pin ${pinname} direction=${pindirection}"
                }
                myputs ${fileout} ""

                foreach device $devices {
                    dump_lgf_placed_device $fileout $cell $device
                }
                myputs ${fileout} ""
            }
            # end of of LGF
        }

        # dump hierarchy
        foreach inst $insts {
            
            if {[is_a_device $cell $inst]} { continue }
            
            set imaster    [inst_get_master_cell $inst]
            set iname      [inst_get_name        $inst]
            set itransform [inst_get_transform   $inst]

            set hierarchy2 $hierarchy
            lappend hierarchy2 $inst

            cell_dump_flat_layout $fileout $imaster $hierarchy2 $optionDumpTopLevelNetsOnly $optionSkipSynNets $optionTheOnlyNetToDump
        }

        if {$dumpDevicePolygons} {
            
            ##################################################################################################################

            set geometries [list]

            foreach device $devices {

                set diffpolygons [udm_utils_get_diff_polygons $device]
                set gatepolygons [udm_utils_get_gate_polygons $device]
                
                set polygons [list]
                foreach polygon $diffpolygons { lappend polygons $polygon }
                foreach polygon $gatepolygons { lappend polygons $polygon }
                
                set netname ""
                error "not implemented yet"
                
                foreach polygon $polygons {
                    
                    set geometry [polygon_apply_transformation $polygon $hierarchy]
                    if {$geometry == ""} { continue }

                    geometry_set_net geometry $netname
                    lappend geometries $geometry
                    lappend polygongeometriesAll $geometry
                }
            }
            
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} device polygons: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }
        
        if {$dumpNetLayoutObjs} {
            
            ##################################################################################################################

            set geometries [list]
            
            foreach net $nets {
                
                set lobjs [net_get_layout_objs $net]
                if {[llength $lobjs] == 0} { continue }

                if {$optionSkipSynNets && [net_is_syn $net]} { continue }
                if {$optionDumpTopLevelNetsOnly && ![net_is_top_level $net $hierarchy]} { continue }
                
                set netname [net_get_top_level_name $net $hierarchy]

                if {$optionTheOnlyNetToDump != "" && $netname != $optionTheOnlyNetToDump} { continue }
                
                foreach layoutobj $lobjs {
                    
                    set geoobj $layoutobj
                    
                    set polygons [geo_obj_get_polygons $geoobj]
                    
                    foreach polygon $polygons {

                        set geometry [polygon_apply_transformation $polygon $hierarchy]
                        if {$geometry == ""} { continue }

                        geometry_set_net geometry $netname
                        lappend geometries $geometry
                        lappend polygongeometriesAll $geometry
                    }
                }
            }

            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} net layout objects: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                foreach inst $hierarchy {
                    set transform  [inst_get_transform   $inst]
                    set imaster    [inst_get_master_cell $inst]
                    set instname   [inst_get_name        $inst]
                    set mastername [cell_get_name        $imaster]
                    myputs ${fileout} "#   instname=${instname} mastername=${mastername} transform=${transform}"
                }
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }

        if {$dumpNetGeoPolygons} {

            ##################################################################################################################

            set geometries [list]
            
            foreach net $nets {

                set geopolygons [net_get_geo_polygons $net]
                if {[llength $geopolygons] == 0} { continue }

                if {$optionSkipSynNets && [net_is_syn $net]} { continue }
                if {$optionDumpTopLevelNetsOnly && ![net_is_top_level $net $hierarchy]} { continue }

                set netname [net_get_top_level_name $net $hierarchy]
                
                if {$optionTheOnlyNetToDump != "" && $netname != $optionTheOnlyNetToDump} { continue }
                
                foreach geopolygon $geopolygons {
                    
                    set polygon [geo_polygon_get_polygon $geopolygon]
                    
                    set geometry [polygon_apply_transformation $polygon $hierarchy]
                    if {$geometry == ""} { continue }
                    
                    geometry_set_net geometry $netname
                    lappend geometries $geometry
                    lappend polygongeometriesAll $geometry
                }
            }
        
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} net geo polygons: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }
        
        if {$dumpCellLayoutObjs} {

            ##################################################################################################################

            set geometries [list]
            
            foreach layoutobj $celllayoutobjs {

                set net    [layout_obj_get_net    $layoutobj]
                set ported [layout_obj_get_ported $layoutobj]
                
                if {$optionSkipSynNets && [net_is_syn $net]} { continue }
                if {$optionDumpTopLevelNetsOnly && ![net_is_top_level $net $hierarchy]} { continue }
                
                set netname [net_get_top_level_name $net $hierarchy]
                
                if {$optionTheOnlyNetToDump != "" && $netname != $optionTheOnlyNetToDump} { continue }
                
                set geoobj $layoutobj

                set polygons [geo_obj_get_polygons $geoobj]
                
                foreach polygon $polygons {

                    set geometry [polygon_apply_transformation $polygon $hierarchy]
                    if {$geometry == ""} { continue }

                    geometry_set_net    geometry $netname
                    geometry_set_ported geometry $ported

                    lappend geometries $geometry
                    lappend polygongeometriesAll $geometry
                }
            }
        
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} cell layout objects: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }

        if {$dumpCellGeoPolygons} {

            ##################################################################################################################

            set geometries [list]

            foreach geopolygon $cellgeopolygons {

                set net [geo_polygon_get_net $geopolygon]
                
                if {$optionSkipSynNets && [net_is_syn $net]} { continue }
                if {$optionDumpTopLevelNetsOnly && ![net_is_top_level $net $hierarchy]} { continue }
                
                set netname [net_get_top_level_name $net $hierarchy]
                
                if {$optionTheOnlyNetToDump != "" && $netname != $optionTheOnlyNetToDump} { continue }
                
                foreach geopolygon $geopolygons {
                    
                    set polygon [geo_polygon_get_polygon $geopolygon]
                    
                    set geometry [polygon_apply_transformation $polygon $hierarchy]
                    if {$geometry == ""} { continue }

                    geometry_set_net geometry $netname
                    lappend geometries $geometry
                    lappend polygongeometriesAll $geometry
                }
            }
        
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} cell geo polygons: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }
        
        if {$dumpCellGeoObjs} {

            ##################################################################################################################

            set geometries [list]

            foreach geoobj $cellgeoobjs {

                if {$optionSkipSynNets} { error "unexpected to have this option here" }
                if {$optionDumpTopLevelNetsOnly} { error "unexpected to have this option here" }
                
                set netname [create_net_hier_name "!float" $hierarchy]
                
                set polygons [geo_obj_get_polygons $geoobj]

                foreach polygon $polygons {

                    set geometry [polygon_apply_transformation $polygon $hierarchy]
                    if {$geometry == ""} { continue }
                    
                    if {[geometry_get_layer $geometry] == [layer_null]} { continue }
                    if {[geometry_is_redundant $geometry polygongeometriesAll]} { continue }

                    geometry_set_net geometry $netname
                    lappend geometries $geometry
                    lappend polygongeometriesAll $geometry
                }
            }
        
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} cell geo objects: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }

            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }

        if {$dumpCellPolygons} {

            ##################################################################################################################

            set geometries [list]
            
            foreach polygon $cellpolygons {

                if {$optionSkipSynNets} { error "unexpected to have this option here" }
                if {$optionDumpTopLevelNetsOnly} { error "unexpected to have this option here" }
                
                set netname [create_net_hier_name "!float" $hierarchy]
                
                set geometry [polygon_apply_transformation $polygon $hierarchy]
                if {$geometry == ""} { continue }

                if {[geometry_get_layer $geometry] == [layer_null]} { continue }
                if {[geometry_is_redundant $geometry polygongeometriesAll]} { continue }

                geometry_set_net geometry $netname
                lappend geometries $geometry
                lappend polygongeometriesAll $geometry
            }
            
            if {[llength $geometries] > 0} {
                myputs ${fileout} ""
                myputs ${fileout} "# Cell ${hiercellname} \{$cellname\} cell polygons: [llength $geometries]; hierarchy level: [llength $hierarchy]"
                myputs ${fileout} "#"
                myputs ${fileout} ""
            }
            
            foreach geometry $geometries {
                geometry_dump $fileout $geometry
            }
        }
    }
    # end of dump_flat_layout

    #
    proc is_a_device {cell inst} {

        set devices [cell_get_devices $cell]
        
        if {[lsearch -exact $devices $inst] >= 0} { return 1 }
        
        return 0
    }
    # end of is_a_device

    #
    proc calculate_device_placement {bbox type r_ref x_ref y_ref} {

        upvar $r_ref r
        upvar $x_ref x
        upvar $y_ref y

        #puts $bbox
        
        set i 0
        set xl [lindex $bbox $i] ; incr i
        set yl [lindex $bbox $i] ; incr i
        set xh [lindex $bbox $i] ; incr i
        set yh [lindex $bbox $i] ; incr i

        set xc [expr {($xl + $xh) / 2}]
        
        if {[expr {$xc % $::_polyPitch}] != 0} { error "unexpected xc=$xc" }

        set x [expr {$xc / $::_polyPitch - 1}]

        set ostatok [expr {$yl % $::_rowHeight}]
        set r [expr {($yl - $ostatok) / $::_rowHeight}]

        set y 0
        
    }
    # end of calculate_device_placement

    #
    proc dump_lgf_placed_device {fileout cell device} {

        set devicename [igen_get_name $device]
        
        set str "Device"

        # net name (order is important)
        append str " ${devicename}"

        # nets
        append str " nets=\["

        set pinbboxes [list]
        set pinnets   [list]

        # source
        # gate
        # drain
        # bulk
        foreach dp [list s g d b] {
            
            set ipin [inst_get_ipin $device $dp]
            set net [ipin_get_external_net $ipin]
            append str " [net_get_name $net]"
            lappend pinnets $net

            # collect pin bboxes
            if {$dp == "s" || $dp == "g" || $dp == "d"} {
                set gig [lindex [igen_get_polygons_pi $device $ipin] 0]
                lappend pinbboxes [gig_figure_get_bbox $gig]
            }
        }
        append str " \]"
        
        set type [udm_utils_get_device_type $device]
        append str " type=${type}"

        set model [udm_utils_get_device_model $device]
        
        foreach iter $::_modelMap {

            set model0 [lindex $iter 0]
            set model1 [lindex $iter 1]

            if {$model == $model0} {
                set model $model1
                break
            }
        }

        append str " model=${model}"

        set width [udm_utils_get_device_width $device]
        set width [scale_bbox $width]
        append str " w=${width}"

        set length [udm_utils_get_device_length $device]
        set length [scale_bbox $length]
        append str " l=${length}"

        # active region
        set polygon [new_polygon]
        udm_utils_get_active_region $device $polygon
        set bbox [gig_figure_get_bbox $polygon]
        set bbox [scale_bbox $bbox]
        
        set bboxcoords [split $bbox ":"]  
        if {[llength $bboxcoords] != 4} { error "" }
        
        # architecture specific info; you need to convert active_region_bbox, row height, nwell south border to r/x/y
        set r "<TBD>"
        set x "<TBD>"
        set y "<TBD>"

        calculate_device_placement $bboxcoords $type r x y

        set spinbbox [lindex $pinbboxes 0]
        set dpinbbox [lindex $pinbboxes 2]
        set sxl [lindex [split $spinbbox ":"] 0]
        set dxl [lindex [split $dpinbbox ":"] 0]

        if {$sxl < $dxl} {
            set f "d" ; # direct
        } else {
            set f "i" ; # inverted
        }
        
        append str " r=${r} x=${x} y=${y} f=${f}"

        set nesting [get_device_nesting $cell $device]
        if {$nesting != ""} {
            append str " nesting=${nesting}"
        }

        if {$::_skipDummyDevices} {

            set snet [lindex $pinnets 0]
            set gnet [lindex $pinnets 1]
            set dnet [lindex $pinnets 2]

            set snetname [net_get_name $snet]
            set gnetname [net_get_name $gnet]
            set dnetname [net_get_name $dnet]

            set isdummy 0

            if {$snetname == $dnetname && [string range $gnetname 0 2] == "syn"} { set isdummy 1 }
            if {$snetname == $dnetname && $snetname == $gnetname} { set isdummy 1 }
            if {[string range $snetname 0 2] == "syn" && [string range $gnetname 0 2] == "syn" && [string range $dnetname 0 2] == "syn"} { set isdummy 1 }

            if {$isdummy} {
                puts "Skipped dummy device: ${str}"
                return
            }
        }

        # print the device
        myputs ${fileout} "  ${str}"
    }
    # end of dump_lgf_placed_device

    #
    proc cell_dump_netlist {cell {fileout ""}} {

        set cellname [cell_get_name $cell]

        set added [add_if_unique $cellname ::_cellMasters]
        if {!$added} { return }

        puts "Found a new cell master $cellname"
        
        set devices [cell_get_devices      $cell]
        set nets    [cell_get_nets         $cell]
        set insts   [cell_get_child_insts  $cell]
        set pins    [cell_get_pins         $cell]
        
        myputs ${fileout} "CellMaster name=${cellname} {"
        myputs ${fileout} ""
        
        foreach net $nets {

            set netname [net_get_name $net]
            #myputs ${fileout} "  Net name=${netname}"
        }

        foreach pin $pins {

            set pinname      [pin_get_name $pin]
            set pindirection [pin_get_direction $pin]

            myputs ${fileout} "  Pin name=${pinname} direction=${pindirection}"
        }
        myputs ${fileout} ""

        set devicenames [list]

        foreach device $devices {
            
            set devicename [igen_get_name $device]
            lappend devicenames $devicename

            # non-LGF netlist
            if {1} {
                set devicemaster [igen_get_master_cell $device]
                set devicemastername [cell_get_name $devicemaster]
                myputs ${fileout} "  Device type=${devicemastername} name=${devicename}"
            } else {
                dump_lgf_placed_device $fileout $cell $device
            }
        }
        
        # store all cell masters here
        set cellmasters [list]
        
        foreach inst $insts {
            
            set imaster    [inst_get_master_cell $inst]
            set iname      [inst_get_name        $inst]
            set itransform [inst_get_transform   $inst]
            set iboundary  [inst_get_boundary    $inst]
            set ipins      [inst_get_ipins       $inst]
            
            # skip devices
            if {[lsearch -exact $devicenames $iname] >= 0} { 
                continue
            }
            
            lappend cellmasters $imaster
            
            set imastername [cell_get_name $imaster]
            set ibbox [gig_figure_get_bbox $iboundary] ; # absolute coords

            myputs ${fileout} "  CellInstance master=${imastername} name=${iname} {"

            foreach ipin $ipins {

                set ipinextnet [ipin_get_external_net $ipin]
                if {$ipinextnet == ""} { continue }

                set ipinextnetname [net_get_name $ipinextnet]

                set ipinmasterpin [ipin_get_master_pin $ipin]
                set ipinmasterpinname [pin_get_name $ipinmasterpin]

                myputs ${fileout} "    PinInstance pin=${ipinmasterpinname} net=${ipinextnetname}"
            }
            
            myputs ${fileout} "  }"
            myputs ${fileout} ""
        }
        
        myputs ${fileout} "}"
        myputs ${fileout} ""

        foreach imaster $cellmasters {
            cell_dump_netlist $imaster $fileout
        }
        
    }
    # end of cell_dump_netlist

    #
    proc dump_netlist {{filename ""}} {

        set ::_cellMasters [list]
        
        set cell [get_active_cell]
        if {$cell == ""} {
            error "No active cell"
        }

        puts "Found active cell [cell_get_name $cell]"

        set fileout ""
        if {$filename != ""} {
            set fileout [open $filename "w"]
            if {$fileout == ""} {
                error "can't open file $filename for reading"
            }
        }

        cell_dump_netlist $cell $fileout

        if {$fileout != ""} {
            close $fileout
            puts "Written $filename"
        }
    }
    # end of dump_netlist

    #
    proc dump_flat_layout {{filename ""} {optionDumpTopLevelNetsOnly 0} {optionSkipSynNets 0} {optionTheOnlyNetToDump ""}} {

        set cell [get_active_cell]
        if {$cell == ""} {
            error "No active cell"
        }

        puts "Found active cell [cell_get_name $cell]"

        set fileout ""
        if {$filename != ""} {
            set fileout [open $filename "w"]
            if {$fileout == ""} {
                error "can't open file $filename for reading"
            }
        }

        set transform [cell_get_transform_up $cell]
        
        set hierarchy [list]
                
        cell_dump_flat_layout $fileout $cell $hierarchy $optionDumpTopLevelNetsOnly $optionSkipSynNets $optionTheOnlyNetToDump
        
        if {$fileout != ""} {
            close $fileout
            puts "Written $filename"
        }
    }
    # end of dump_flat_layout

    #
    proc dump_generators {{filename ""}} {

        set gigenmgr [gigen_mgr_get_mgr]
        set gigens [gigen_mgr_get_generators $gigenmgr]
        puts "Found [llength $gigens] generators"

        set fileout ""
        if {$filename != ""} {
            set fileout [open $filename "w"]
            if {$fileout == ""} {
                error "can't open file $filename for reading"
            }
        }

        foreach gigen $gigens {

            set name [gigen_get_name $gigen]

            #set pars [gigen_get_op_params $gigen "CREATEWIDTHPOINT"]
            set pars [gigen_get_op_params $gigen "CREATEWIDTHPOINT"]

            if {$fileout != ""} {
                puts $fileout "Generator name=${name} \{"
            }
            
            foreach par $pars {

                set propertydescriptor [gen_par_get_prop_descriptor $par]
                set propertydescriptorname  [property_descriptor_get_name  $propertydescriptor]
                set propertydescriptorvalue [property_descriptor_get_value $propertydescriptor 0]

                if {$fileout != ""} {
                    puts $fileout "  Property name=${propertydescriptorname} value=${propertydescriptorvalue}"
                }
            }

            if {$fileout != ""} {
                puts $fileout "\}"
            }
        }

        if {$fileout != ""} {
            close $fileout
            puts "Written $filename"
        }
    }
    # end of dump_generators

    #
    proc param_split_hier_name {param} {
        
        return [split [param_get_hier_name $param] "."]
    }
    # end of param_split_hier_name

    #
    proc param_split_value {param} {

        return [split [param_get_value $param] ";"]
    }
    # end of param_split_value

    #
    proc param_get_hier_name {param} {

        return [lindex $param 0]
    }
    # end of param_get_hier_name

    #
    proc param_get_parent_name {param} {

        set hiername [param_get_hier_name $param]
        set index [string last "." $hiername]
        if {$index < 0} { return "" }

        incr index -1

        return [string range $hiername 0 $index]
    }
    # end of param_get_parent_name

    #
    proc param_get_name {param} {

        set names [param_split_hier_name $param]
        set numnames [llength $names]
        if {$numnames <= 0} { error "bad names: $names" }

        return [lindex $names [expr {$numnames - 1}]]
    }
    # end of param_get_name

    #
    proc param_get_value {param} {

        return [lindex $param 1]
    }
    # end of param_get_value

    #
    proc param_get_value_as_list {param} {
        
        set delim ","
        
        set values [param_split_value $param]
        
        set numvalues [llength $values]
        if {$numvalues <= 0} { error "bad values: $values" }

        set str "[lindex $values 0]"

        for {set i 1} {$i < $numvalues} {incr i} {
            set value [lindex $values $i]
            append str "${delim}${value}"
        }

        return $str
    }
    # end of param_get_value_as_list

    #
    proc param_get_level {param} {

        set parnames [param_split_hier_name $param]
        set level [llength $parnames]
        incr level -1
        if {$level < 0} { error "bad level" }

        return $level
    }
    # end of param_get_level

    #
    proc parse_params_as_tokens {parenttoken parentvalue level paramsPerLevel_ref} {

        upvar $paramsPerLevel_ref paramsPerLevel
        if {[llength [array names paramsPerLevel -exact $level]] == 0} { return }

        set params $paramsPerLevel($level)
        set remainedparams [list]

        foreach param $params {

            set name       [param_get_name $param]
            set parentname [param_get_parent_name $param]
            set value      [param_get_value_as_list $param]

            if {$parentvalue != ""} {
                if {$parentname != $parentvalue} {
                    lappend remainedparams $param
                    continue
                }
            }

            #puts $fileout "${prefix}${name} value=${value} \{"
            set token [::tkn::create_token $name]
            $token add_par_val "value" $value
            $parenttoken add_token $token

            if {[llength $value] == 1} {
                
                set nextparentvalue ${parentvalue}
                if {$nextparentvalue != ""} {
                    append nextparentvalue "."
                }
                append nextparentvalue $value

                set nextlevel [expr {$level + 1}]
                parse_params_as_tokens $token $nextparentvalue $nextlevel paramsPerLevel
            }

            #puts $fileout "${prefix}\}"
        }

        if {[llength $remainedparams] != 0} {
            set paramsPerLevel($level) $remainedparams
        } else {
            array unset paramsPerLevel $level
        }
    }
    # end of parse_params_as_tokens
    
    #
    proc set_tech {techname} {

        set ::_tech $techname
    }
    # end of set_tech 

    #
    proc set_car {carfilename} {

        if {![file exists $carfilename]} {
            error "set_car: CAR file doesn't exist: $carfilename"
        }
        
        puts "set_car: Set CAR file $carfilename"
        ::car::car_mgr_create $carfilename

        print_active_car
    }
    # end of set_car

    #
    proc set_auto_car {} {
        
        set carfilename [::boo::get_path $::_tech "car"]
        
        if {$carfilename == ""} { 
            puts "set_auto_car: Couldn't get CAR file path for process $::_tech"
            return  
        }
        
        puts "set_auto_car: Found CAR file $carfilename"
        set_car $carfilename
    
    }
    # end of set_auto_car

    #
    proc print_active_car {} {

        set car [::car::car_mgr_get_car]
        if {$car == ""} { puts "print_active_car: No active CAR found" }

        set carfile [car::car_get_car_file_name $car]
        puts "print_active_car: Active car belongs to a file: $carfile"         
    }
    # end of print_active_car
    
    #
    proc dump_generators_from_car_file {{outfilename ""}} {

        set car [::car::car_mgr_get_car]
        if {$car == ""} { error "No active CAR found" }
        
        set fileout ""
        if {$outfilename != ""} {
            set fileout [open $outfilename "w"]
            if {$fileout == ""} {
                error "can't open file $outfilename for writing"
            }
        }

        set carObjs [::car::car_get_generic_objects $car]
        set date [exec date]
        set user $::env(USER)

        if {$fileout != ""} {
            
            puts $fileout "# Auto-generated file"
            puts $fileout "# Date: [exec date]"
            puts $fileout "# User: $::env(USER)"
            puts $fileout "# Car file: [car::car_get_car_file_name $car]"
            puts $fileout "# Generated by: dump_generators_from_car_file (utils.tcl)"
            puts $fileout "# Disclaimer: This file is not intended to be reader friendly"
            puts $fileout "# Check any issues at the end of the file"
            puts $fileout ""
        }

        set messages [list]

        foreach carObj $carObjs {
            
            set type [::car::car_generic_object_get_type $carObj]
            set name [::car::car_generic_object_get_name $carObj]

            #puts $fileout "# $type $name"
            #continue

            if {$type != "VIA"} { 
                lappend messages "Skipped generator $name: unexpected type: $type"
                continue
            }
            
            if {![regexp "via.*" $name match] && ![regexp "Via.*" $name match]} {
                lappend messages "Skipped generator $name: unexpected name: $name"
                continue
            }
            
            set carobjparams [::car::car_generic_object_get_params $carObj]

            array set paramsPerLevel {}

            foreach param $carobjparams {

                set level [param_get_level $param]
                if {$level < 0} { error "bad level" }

                if {[llength [array names paramsPerLevel -exact $level]] == 0} {
                    set paramsPerLevel($level) [list]
                }
                lappend paramsPerLevel($level) $param
            }

            if {[llength [array names paramsPerLevel -exact 0]] == 0} {
                error "Generator name=${name} has no params at level 0"
            }
            
            if {$fileout != ""} {

                foreach {key value} [array get paramsPerLevel] {

                    set level $key
                    set params $value

                    puts $fileout "# Found [llength $params] params at level ${level}: ${params}"
                }
                
                # use Token class to store data
                set token [::tkn::create_token "Generator"]
                $token add_par_val "name" $name

                parse_params_as_tokens $token "" 0 paramsPerLevel

                foreach {key value} [array get paramsPerLevel] {
                    
                    set level $key
                    set params $value
                    puts $fileout "# Warning: remained [llength $params] params at level ${level}: ${params}"

                    if {$level == 1} {
                        puts $fileout "# Warning: Dumped at level [expr {$level - 1}] instead of level ${level}"
                        parse_params_as_tokens $token "" 1 paramsPerLevel
                    }
                }

                modify_generator_if_it_is_not_legal_enough_ $token
                
                $token print_tokens $fileout
                puts $fileout ""

                catch {delete object $token}
            }

            array unset paramsPerLevel
        }

        if {$fileout != ""} {

            puts $fileout "#"
            puts $fileout "# Found [llength $messages] issues"
            puts $fileout "#"

            foreach msg $messages {
                puts $fileout "# $msg"
            }

            close $fileout
            
            puts "Written $outfilename"
        }
    }
    # end of dump_generators_from_car_file

    # I am not sure gigen_generators store the via cut/enclosure data statically. I believe they load it from car file dynamically.
    # I am guessing you are trying to read via definitions.
    # I typically do this by loading data from .car file
    # Please see the helper function below for a sample implementation. Hope it helps.
    proc getAllLegalViasFromCar {} {
        
        set carFile [::boo::get_path $::_tech car]
        if { $carFile == "" } { return }
        array unset ::mr::viaArray
        array set ::mr::viaArray {}
        set car [::car::car_mgr_get_car]
        if { $car == "" } {
            set car [::car::car_mgr_create $carFile]
        }
        set carObjs [::car::car_get_generic_objects $car]
        foreach carObj $carObjs {
            set name [::car::car_generic_object_get_name $carObj]
            set type [::car::car_generic_object_get_type $carObj]
            if { $type == "VIA" && [regexp "via.*" $name match] } {
                #puts "processing via $name"
                set layer1 [::car::car_generic_object_get_param $carObj "Layer1"]
                set layer2 [::car::car_generic_object_get_param $carObj "Layer2"]
                set CutWidth [::car::car_generic_object_get_param $carObj "CutWidth"]
                set CutHeight [::car::car_generic_object_get_param $carObj "CutHeight"]
                set XCutSpacing [::car::car_generic_object_get_param $carObj "XCutSpacing"]
                set YCutSpacing [::car::car_generic_object_get_param $carObj "YCutSpacing"]
                set l1Widths [::car::car_generic_object_get_param $carObj "${layer1}.widths"]
                set l2Widths [::car::car_generic_object_get_param $carObj "${layer2}.widths"]
                set l1Widths [split $l1Widths ";"]
                set l2Widths [split $l2Widths ";"]
                set CutHeightUdm [tech_micron_to_udm $::_tech $CutHeight]
                set CutWidthUdm [tech_micron_to_udm $::_tech $CutWidth]
                set XCutSpacingUdm [tech_micron_to_udm $::_tech $XCutSpacing]
                set YCutSpacingUdm [tech_micron_to_udm $::_tech $YCutSpacing]
                foreach l1Width $l1Widths {
                    set layer1WidthUdm [tech_micron_to_udm $::_tech $l1Width]
                    foreach l2Width $l2Widths {
                        set layer2WidthUdm [tech_micron_to_udm $::_tech $l2Width]
                        lappend ::mr::viaArray($layer1#$layer2#$layer1WidthUdm#$layer2WidthUdm) "$CutWidthUdm#$CutHeightUdm#$XCutSpacingUdm#$YCutSpacingUdm"
                    }
                }
            }
        }
        #::mr::populateMinViaCuts
        ::mr::populateMaxViaCuts
    }
    # end of getAllLegalViasFromCar

    #
    proc modify_generator_if_it_is_not_legal_enough_ {token} {

        # used this procedure to test some ideas
        #modify_generator_fix_metal3_widths_for_via3_ $token
        #anton: June12, 2018. Add missing data
        modify_generator_fix_missing_cut_layer_ $token
        modify_generator_fix_missing_coverage_ $token
        
    }
    # end of modify_generator_if_it_is_not_legal_enough_

    #
    proc modify_generator_fix_metal3_widths_for_via3_ {token} {

        # get and check cut layer
        set cutlayers [list "viaa3" "viab3"]
        set cutlayer [$token get_value_of_a_single_subtoken "cutlayer" "value"]
        if {[lsearch -exact $cutlayers $cutlayer] < 0} { return }

        # CutWidth
        set cutwidth [$token get_value_of_a_single_subtoken "CutWidth" "value"]
        if {$cutwidth <= 0.0} { error "" }

        # get and double check Layer1
        set layers1 [list "metal3" "metalc3"]
        set layer1 [$token get_value_of_a_single_subtoken "Layer1" "value"]
        if {[lsearch -exact $layers1 $layer1] < 0} {
            $token print_tokens
            error "unexpected generator"
        }
        
        # layer1 token
        set layer1_token [$token get_single_token "Layer1"]
        set layer1_widths_token [$layer1_token get_single_token "widths"]
        set layer1_widths [$layer1_widths_token get_value_as_list "value"]
        set layer1_x_coverage [$layer1_token get_value_of_a_single_subtoken "x_coverage" "value"]
        
        # actions
        $layer1_widths_token set_name "nikolai_original_car_widths"
        set layer1_updated_widths_token [$layer1_token add_new_token "widths"]

        # minimal landing width
        set minimal_landing_width [expr {$cutwidth + 2 * $layer1_x_coverage}]
        set minimal_landing_width_udm [tech_micron_to_udm $::_tech $minimal_landing_width]

        # prune illegal widths
        set newwidths ""
        foreach width $layer1_widths {
            set width_udm [tech_micron_to_udm $::_tech $width]
            
            # metal3 is too wide for this small via3 cut
            if {$width_udm > $minimal_landing_width_udm} { continue }
            
            if {$newwidths != ""} {
                append newwidths ","
            }
            append newwidths $width
        }
        if {[llength $newwidths] == 0} { error "bad new widths" }
        $layer1_updated_widths_token add_par_val "value" $newwidths

    }
    # end of modify_generator_fix_metal3_widths_for_via3_
    
    proc modify_generator_fix_missing_cut_layer_ {token} {


        # get and check cut layer
        set subtokens [$token get_tokens "cutlayer"]
        
        if {[llength $subtokens] != 0} {
            return
        }

        set topName  [$token get_value  "name"]
        set found [regexp {^via[0-9]+} $topName cutLayer]
        if {!$found} {
            error "Failed to get via name from generator name $topName"
        }
        #puts "generator $topName -> cutlayer $cutLayer"
        set cutlayer_token [$token add_new_token "cutlayer"]
        $cutlayer_token add_par_val "value" $cutLayer


    }
    # end of modify_generator_fix_missing_cut_layer_
    
    proc modify_generator_fix_missing_coverage_ {token} {

      foreach layerName {"Layer1" "Layer2"} {
        set layer_token [$token get_single_token $layerName]
        foreach tokenName {"x_coverage" "y_coverage"} {
        
          # get and check cut layer
          set subtokens [$layer_token get_tokens $tokenName]
        
          if {[llength $subtokens] != 0} {
            continue
          }

          set topName  [$token get_value  "name"]
          #puts "generator $topName/$layerName -> $tokenName 0"
          set cutlayer_token [$layer_token add_new_token $tokenName]
          $cutlayer_token add_par_val "value" "0.0"
        }
      }


    }
    # end of modify_generator_fix_missing_coverage_
    
}
# end of namespace

#::cln::set_car $::_carFileName
