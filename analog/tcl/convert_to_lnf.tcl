#set ::env(AMSRDIR) /nfs/sc/disks/scl.work.01/ppt/nvryzhen/cellGen/fls/analog
#set ::env(AMSRDIR) /nfs/sc/disks/scl.work.01/ppt/nvryzhen/releases/amsr

if {[array names ::env AMSRDIR] == ""} {
    error "Environment variable AMSRDIR is not set."
}

set filenames [list]
lappend filenames $::env(AMSRDIR)/tcl/token.tcl

foreach filename $filenames {
    if {![file exists $filename]} {
        error "File does not exist: $filename"
    }
    puts "source $filename"
    source $filename
}

namespace eval ::cln {
    
    # hardcode to most active tech for a while
    #set ::_tech "p1274.1"
    set ::_tech "p1273"

    set ::_scale 1

    #
    proc set_scale {value} {
        
        set ::_scale $value
        
        puts "Scale set to $::_scale"
    }
    # end of set_scale

    # primitive: rawwire
    #
    
    proc rawwire_create {net layer dcoords} { return [list $net $layer [lindex $dcoords 0] [lindex $dcoords 1] [lindex $dcoords 2] [lindex $dcoords 3]] }
    
    proc rawwire_get_net   {obj} { return [lindex $obj 0] }
    proc rawwire_get_layer {obj} { return [lindex $obj 1] }
    proc rawwire_get_xl    {obj} { return [lindex $obj 2] }
    proc rawwire_get_yl    {obj} { return [lindex $obj 3] }
    proc rawwire_get_xh    {obj} { return [lindex $obj 4] }
    proc rawwire_get_yh    {obj} { return [lindex $obj 5] }
    
    # primitive: rawobj
    #

    proc rawobj_create {net generator x y rawwires} { return [list $net $generator $x $y $rawwires] }

    proc rawobj_get_net       {obj} { return [lindex $obj 0] }
    proc rawobj_get_generator {obj} { return [lindex $obj 1] }
    proc rawobj_get_x         {obj} { return [lindex $obj 2] }
    proc rawobj_get_y         {obj} { return [lindex $obj 3] }
    proc rawobj_get_rawwires  {obj} { return [lindex $obj 4] }

    # primitive: rawdevice
    #

    proc rawdevice_create {name snet gnet dnet bnet type model width length row x y flip} { return [list $name $snet $gnet $dnet $bnet $type $model $width $length $row $x $y $flip] }

    proc rawdevice_get_name   {v} { return [lindex $v  0] }
    proc rawdevice_get_snet   {v} { return [lindex $v  1] }
    proc rawdevice_get_gnet   {v} { return [lindex $v  2] }
    proc rawdevice_get_dnet   {v} { return [lindex $v  3] }
    proc rawdevice_get_bnet   {v} { return [lindex $v  4] }
    proc rawdevice_get_type   {v} { return [lindex $v  5] }
    proc rawdevice_get_model  {v} { return [lindex $v  6] }
    proc rawdevice_get_width  {v} { return [lindex $v  7] }
    proc rawdevice_get_length {v} { return [lindex $v  8] }
    proc rawdevice_get_row    {v} { return [lindex $v  9] }
    proc rawdevice_get_x      {v} { return [lindex $v 10] }
    proc rawdevice_get_y      {v} { return [lindex $v 11] }
    proc rawdevice_get_flip   {v} { return [lindex $v 12] }

    proc canvas_create_wire_from_a_rawwire {cell rawwire layer} {

        set scale $::_scale

        # it's slow; so that I use externally pre-computed layer
        if {$layer == ""} {

            set tech [cell_get_tech $cell]
            set techName [tech_get_name $tech]
            set layername [rawwire_get_layer $rawwire]
            
            set layer [string_to_tech_layer_ $layername $techName]
        }

        if {$layer == ""} {
            return ""
        }

        set netname [rawwire_get_net   $rawwire]
        set xl      [rawwire_get_xl    $rawwire]
        set yl      [rawwire_get_yl    $rawwire]
        set xh      [rawwire_get_xh    $rawwire]
        set yh      [rawwire_get_yh    $rawwire]

        set net [get_cell_net_ $cell $netname]
        
        set rect [new_rect_from_xy [expr $xl * $scale] [expr $yl * $scale] [expr $xh * $scale] [expr $yh * $scale] $layer]

        set wire [wire_create $net $rect]

        return $wire
    }
    # end of canvas_create_wire_from_a_rawwire

    #
    proc read_wires_from_lgf_file {filename} {

        set cell [get_active_cell]
        if {$cell == ""} {
            error "No active cell. Aborted."
        }

        set bbox ""
        set rawwires   [list]
        set rawobjs    [list]
        set rawdevices [list]

        read_raw_objects_from_file_ $filename bbox rawwires rawobjs rawdevices

        import_raw_objects_to_cell_ $cell rawwires rawobjs rawdevices

        catch { ::cadworks::redraw }
    }
    # end of read_wires_from_lgf_file

    #
    proc print_techs {} {

        set cellmgr [cell_mgr_get_mgr]

        set techs [cell_mgr_get_techs $cellmgr]

        puts "print_techs: Available techs: $techs"
    }
    # end of print_techs
    
    #
    proc convert_to_lnf {filename cellname} {

        puts "tech $::_tech"

        if {![file exists $filename]} {
            error "convert_to_lnf: File does not exist: ${filename}"
        }
        
        # normalize
        set filename [file normalize $filename]
        if {![file exists $filename]} {
            error "convert_to_lnf: File does not exist: ${filename}"
        }
        
        # dummy check
        if {[file isdirectory $filename]} {
            error "convert_to_lnf: File is a directory: ${filename}"
        }

        # get root name 
        # Returns all of the characters in name up to but not including the last “.” character in the last component of name.
        # If the last component of name does not contain a dot, then returns name.
        set rootname [file rootname $filename]

        # create LNF dir
        set lnfdir $::env(WARD)/genesys/lnf
        if {![file exists $lnfdir]} {
            file mkdir $lnfdir
        }
        if {![file isdirectory $lnfdir]} {
            error "convert_to_lnf: Not a directory: $lnfdir"
        }

        # check LNF
        set lnfpath ${lnfdir}/${cellname}.lnf
        if {[file exists $lnfpath]} {
            error "convert_to_lnf: LNF file already exists: $lnfpath"
        }
        
        # read input file
        set bbox ""
        set rawwires   [list]
        set rawobjs    [list]
        set rawdevices [list]

        read_raw_objects_from_file_ $filename bbox rawwires rawobjs rawdevices

        # create a new cell
        set cell [create_flat_cell_ $cellname $::_tech $bbox]
        
        # import objects into this cell
        import_raw_objects_to_cell_ $cell rawwires rawobjs rawdevices

        # open and update canvas
        ::cadworks::open -cellname $cellname -viewname "lnf"
        catch { ::cadworks::redraw }

        # copy an error file if it exists
        if {1} {

            set errfileext "playerr"
            set errfile ${rootname}.${errfileext}

            if {[file exists $errfile] && [file isfile $errfile]} {
            
                set errdir $::env(WARD)/genesys/layerr
                if {![file exists $errdir]} {
                    file mkdir $errdir
                }
                if {![file isdirectory $errdir]} {
                    error "convert_to_lnf: Not a directory: $errdir"
                }

                set targeterrfile ${errdir}/${cellname}.${errfileext}

                if {[file exists $targeterrfile]} {
                    error "convert_to_lnf: File already exists: $targeterrfile"
                }

                file copy $errfile $targeterrfile

                puts "Created ${targeterrfile}"
            }
        }
    } 
    # end of convert_to_lnf

    #
    proc read_raw_objects_from_file_ {filename bbox_ref rawwires_ref rawobjs_ref rawdevices_ref} {

        upvar $bbox_ref       bbox
        upvar $rawwires_ref   rawwires
        upvar $rawobjs_ref    rawobjs
        upvar $rawdevices_ref rawdevices

        if {![file exists $filename]} {
            error "File does not exist: $filename"
        }

        set tokenmanager [::tkn::create_token_manager]
        
        set roottoken [$tokenmanager get_root_token]
        
        $tokenmanager parse_token_file $filename

        #$roottoken print_tokens ""

        set celltokens [$roottoken get_tokens "Cell"]
        if {[llength $celltokens] != 1} { error "unexpected number of tokens: Cell" }
        
        set celltoken [lindex $celltokens 0]
        set bbox [$celltoken get_value_as_list "bbox" ":"]
        
        read_wires_from_token_ $celltoken rawwires
        read_wires_from_token_ $roottoken rawwires

        read_objs_from_token_ $celltoken rawobjs
        read_objs_from_token_ $roottoken rawobjs

        read_devices_from_token_ $celltoken rawdevices
        read_devices_from_token_ $roottoken rawdevices

        puts "Scale: $::_scale"
        puts "Bbox: $bbox"
        puts "Found [llength $rawwires] raw wires"
        puts "Found [llength $rawobjs] raw objects"
        puts "Found [llength $rawdevices] raw devices"
        
        if {1} {
            #puts "Delete token manager"
            ::tkn::delete_token_manager $tokenmanager
            #puts "Deleted token manager"
        }
    }
    # end of read_raw_objects_from_file_

    #
    proc read_wires_from_token_ {token rawwires_ref} {

        upvar $rawwires_ref rawwires

        set tokens [$token get_tokens "Wire"]
        #puts "Found [llength $tokens] wire tokens"

        foreach token2 $tokens {
            lappend rawwires [read_wire_ $token2]
        }
    }
    # end of read_wires_from_token_

    #
    proc read_objs_from_token_ {token rawobjs_ref} {

       upvar $rawobjs_ref rawobjs

        set tokens [$token get_tokens "Obj"]
        puts "Found [llength $tokens] obj tokens"

        foreach token2 $tokens {
            lappend rawobjs [read_obj_ $token2]
        }
    }
    # end of read_objs_from_token_

    #
    proc read_devices_from_token_ {token rawdevices_ref} {

        upvar $rawdevices_ref rawdevices

        set tokens [$token get_tokens "Device"]
        puts "Found [llength $tokens] device tokens"

        foreach token2 $tokens {
            lappend rawdevices [read_device_ $token2]
        }
    }
    # end of read_devices_from_token_

    #
    proc read_wire_ {token} {
        
        set net     [$token get_value "net"]
        set layer   [$token get_value "layer"]
        set dcoords [$token get_value_as_list "rect" ":"]

        return [rawwire_create $net $layer $dcoords]
    }
    # end of read_wire_

    #
    proc read_obj_ {token} {
        
        set net       [$token get_value "net"]
        set generator [$token get_value "gen"]
        set x         [$token get_value "x"]
        set y         [$token get_value "y"]

        set rawwires [list]
        read_wires_from_token_ $token rawwires

        return [rawobj_create $net $generator $x $y $rawwires]
    }
    # end of read_obj_

    #
    proc read_device_ {token} {

        # Format:
        # Device obj_1 nets=[ vcc clk nc1 vcc ] type=p model=p w=300 l=280 r=0 x=0 y=0 f=i

        # parse human-styled parameters
        set type   [$token get_value "type"]
        set model  [$token get_value "model"]
        set width  [$token get_value "w"]
        set length [$token get_value "l"]
        set row    [$token get_value "r"]
        set x      [$token get_value "x"]
        set y      [$token get_value "y"]
        set flip   [$token get_value "f"]
        
        # parse evil-styled parameters

        set originalline [$token get_original_line]
        
        # parse name
        set name [lindex $originalline 1]

        # parse nets
        set line $originalline
        set index0 [string first "nets=" $line]
        set line [string range $line $index0 end]
        set index0 [string first "\[" $line]
        set line [string range $line $index0 end]
        set index1 [string first "\]" $line]
        incr index1 -1
        set line [string range $line 1 $index1]
        set line [string trim $line]

        # get nets
        set wnet [lindex $line 0] ; # west
        set gnet [lindex $line 1] ; # gate
        set enet [lindex $line 2] ; # east
        set bnet [lindex $line 3] ; # bulk

        # invert nets here
        if {$flip == "d" || $flip == ""} {
            set snet $wnet ; # source
            set dnet $enet ; # drain
        } elseif {$flip == "i"} {
            set snet $enet ; # source
            set dnet $wnet ; # drain
        } else {
            error "unsupported flip: $flip"
        }

        # override flip
        set flip "d"

        return [rawdevice_create $name $snet $gnet $dnet $bnet $type $model $width $length $row $x $y $flip]
    }
    # end of read_device_

    #
    proc get_boundary_ {cellxl cellyl cellxh cellyh} {
        
        set upperRight [point_from_xy $cellxh $cellyh]
        set lowerRight [point_from_xy $cellxh $cellyl]
        set lowerLeft  [point_from_xy $cellxl $cellyl]
        set upperLeft  [point_from_xy $cellxl $cellyh]
        
        return [polygon_create_from_pts [list $upperRight $lowerRight $lowerLeft $upperLeft] [layer_null]]
    }
    # end of get_boundary_

    #
    proc create_cell_ {name techName bboxxl bboxyl bboxxh bboxyh} {
        
        set cellmgr [cell_mgr_get_mgr]

        set tech [cell_mgr_get_tech $cellmgr $techName]
        if {[::is_null $tech]} { 
            puts "-E- create_cell_: Can't get tech $techName"
            puts "-E- Available techs: [cell_mgr_get_techs $cellmgr]"
            error "Aborted. See messages above."
        }

        set cell [cell_create $name $tech [get_boundary_ $bboxxl $bboxyl $bboxxh $bboxyh] "lnf"]

        if {[::is_null $cell]} { 
            error "create_cell_: Can't create a new cell $name"
        }
        
        #create_default_nets $cell
        
        foreach pin [cell_get_pins $cell] {
        }
        
        return $cell
    }
    # end of create_cell_

    #
    proc get_cell_net_ {cell netname} {

        set net [cell_get_net $cell $netname]
        
        if {[::is_null $net]} {
            set createPin 0
            set net [net_create $cell $netname $createPin]
            if {[::is_null $net]} { error "get_cell_net_: Can't create a new net $netname" }
        }
        
        return $net        
    }
    # end of get_cell_net_

    #
    proc string_to_tech_layer_ {layerName techName} {

        #puts "$layerName"

        # check null layer
        if {$layerName == "null" || $layerName == ""} {
            set layer [layer_null]
            return $layer
        }

        set layer ""
        
        catch { set layer [tech_get_layer $techName $layerName] }
        if {$layer == "" || [is_null $layer]} {
            puts "Warning: Can't get layer $layerName. Skipped."
            return ""
        }

        return $layer
    }
    # end of string_to_tech_layer_

    proc create_car_via_ {carViaName cell net point {xcuts 1} {ycuts 1}} {  
        
        if {$carViaName == {} } {
            return ""
        } 

        if {$cell == ""} {
            error "cell is null"
            return ""
        }

        if {$net == ""} {
            error "net is null"
            return ""
        }

        if {$point == ""} {
            error "point is null"
            return ""
        }
        
        set viagen [gigen_mgr_get_generator [gigen_mgr_get_mgr] $carViaName]        
        if {$viagen == "NULL"} {
            puts "Unable to determine via generator for $carViaName"
            #error "Unable to determine via generator for $carViaName"
            return ""
        }        

        set create_pars [gigen_get_op_params $viagen "CREATEWIDTHPOINT" $cell]
        if { [llength $create_pars] == 0 } {
            return ""
        }

        foreach par $create_pars {

            if { $par == "NULL" } {
                continue
            }
            set propertyDesc [gen_par_get_prop_descriptor $par]
            if { $propertyDesc == "NULL" } {
                continue
            }

            #
            #Set the point
            #
            if { [property_descriptor_get_name $propertyDesc] == "Point1" } {
                gen_par_set_value $par $point
            }
            
            #
            #Set the cuts
            #
            if { [property_descriptor_get_name $propertyDesc] == "xCuts" } {
                gen_par_set_value $par $xcuts
            }
            if { [property_descriptor_get_name $propertyDesc] == "yCuts" } {
                gen_par_set_value $par $ycuts
            }
            if { [property_descriptor_get_name $propertyDesc] == "Net" } {
                gen_par_set_value $par [net_get_name $net]
            }
        }
        
        set layoutobj [gigen_generate $viagen $cell "dummy" $create_pars]
        
        return $layoutobj
    }
    # end of create_car_via_

    #
    proc create_flat_cell_ {cellname techName bbox} {

        if {1} {
            set path $::env(WARD)/genesys/lnf/${cellname}.lnf
            if {[file exists $path]} {
                error "create_flat_cell_: Cell $path already exists."
            }
        }

        set i 0
        set bboxxl [expr {$::_scale * [lindex $bbox $i]}]; incr i
        set bboxyl [expr {$::_scale * [lindex $bbox $i]}]; incr i
        set bboxxh [expr {$::_scale * [lindex $bbox $i]}]; incr i
        set bboxyh [expr {$::_scale * [lindex $bbox $i]}]; incr i
        
        set cell [create_cell_ $cellname $techName $bboxxl $bboxyl $bboxxh $bboxyh]
        
        return $cell
    }
    # end of create_flat_cell_

    #
    proc import_raw_objects_to_cell_ {cell rawwires_ref rawobjs_ref rawdevices_ref} {

        puts "Import raw objects to cell [cell_get_name $cell]"
        
        upvar $rawwires_ref   rawwires
        upvar $rawobjs_ref    rawobjs        
        upvar $rawdevices_ref rawdevices

        set skippedlayers [list]
        set numcreatedwires 0
        set numskippedwires 0

        set tech [cell_get_tech $cell]
        set techName [tech_get_name $tech]

        # put wires per layer
        #
        array set rawwiresPerLayerName {}

        foreach rawwire $rawwires {

            set netname   [rawwire_get_net   $rawwire]
            set layername [rawwire_get_layer $rawwire]
            set xl        [rawwire_get_xl    $rawwire]
            set yl        [rawwire_get_yl    $rawwire]
            set xh        [rawwire_get_xh    $rawwire]
            set yh        [rawwire_get_yh    $rawwire]

            if {[llength [array names rawwiresPerLayerName -exact $layername]] == 0} {
                set rawwiresPerLayerName($layername) [list]
            }
            lappend rawwiresPerLayerName($layername) $rawwire
        }

        # create wires layer by layer
        #

        foreach {key value} [array get rawwiresPerLayerName] {
            
            set layername          $key
            set rawwiresOfTheLayer $value

            if {[lsearch -exact $layername $skippedlayers] >= 0} { 
                incr numskippedwires [llength $rawwiresOfTheLayer]
                continue
            }

            set layer [string_to_tech_layer_ $layername $techName]

            if {$layer == ""} {
                lappend skippedlayers $layername
                incr numskippedwires [llength $rawwiresOfTheLayer]
                continue
            }

            foreach rawwire $rawwiresOfTheLayer {

                canvas_create_wire_from_a_rawwire $cell $rawwire $layer
                
                incr numcreatedwires
            }
        }

        array unset rawwiresPerLayerName

        if {[llength $skippedlayers] != 0} {
            puts "Skipped layers: $skippedlayers"
        }

        puts "Created wires: $numcreatedwires"
        puts "Skipped wires: $numskippedwires"

        # create objects
        #

        foreach rawobj $rawobjs {

            set netname       [rawobj_get_net       $rawobj]
            set generatorname [rawobj_get_generator $rawobj]
            set x             [rawobj_get_x         $rawobj]
            set y             [rawobj_get_y         $rawobj]
            set girawwires    [rawobj_get_rawwires  $rawobj]

            set net [get_cell_net_ $cell $netname]

            set x [expr {$::_scale * $x}]
            set y [expr {$::_scale * $y}]

            set point [point_from_xy $x $y]
            
            set layoutobj [create_car_via_ $generatorname $cell $net $point]
            
            if {$layoutobj == "" || [is_null $layoutobj]} {
                
                puts "can't create a via $generatorname for net [net_get_name $net] at point $point"
                #error "can't create a via $generatorname for net [net_get_name $net] at point $point"
                #continue

                foreach girawwire $girawwires {
                    canvas_create_wire_from_a_rawwire $cell $girawwire ""
                }
            }
        }

        # create devices
        #

        foreach rawdevice $rawdevices {

            create_device_from_raw_device $cell $rawdevice
        }
        
        ::cadworks::save -cellname [cell_get_name $cell] -viewname "lnf"
    }
    # end of import_raw_objects_to_cell_

    #
    proc create_device_from_raw_device {cell rawdevice} {

        set devName [rawdevice_get_name   $rawdevice]
        set devType [rawdevice_get_type   $rawdevice]
        set length  [rawdevice_get_length $rawdevice]
        set width   [rawdevice_get_width  $rawdevice]
        set model   [rawdevice_get_model  $rawdevice]
        set flip    [rawdevice_get_flip   $rawdevice]

        set x   [rawdevice_get_x   $rawdevice]
        set y   [rawdevice_get_y   $rawdevice]
        set row [rawdevice_get_row $rawdevice]

        ##### a quick hard code for one of architectures

        set devxc [expr {($x + 1) * $::_polyPitch}]
        
        if       {[expr {$row % 2}] == 0 && $devType == "n"} { set devyc [expr {$::_rowHeight / 2 - $::_diffPitch / 2 - $width / 2 - $y * $::_diffPitch}]
        } elseif {[expr {$row % 2}] == 0 && $devType == "p"} { set devyc [expr {$::_rowHeight / 2 + $::_diffPitch / 2 + $width / 2 + $y * $::_diffPitch}]
        } elseif {[expr {$row % 2}] == 1 && $devType == "p"} { set devyc [expr {$::_rowHeight / 2 - $::_diffPitch / 2 - $width / 2 - $y * $::_diffPitch}]
        } elseif {[expr {$row % 2}] == 1 && $devType == "n"} { set devyc [expr {$::_rowHeight / 2 + $::_diffPitch / 2 + $width / 2 + $y * $::_diffPitch}]
        } else {
            error "unexpected device placement"
        }

        incr devyc [expr {$::_rowHeight * $row}]

        #####

        set devxc  [expr {$devxc  * $::_scale}]
        set devyc  [expr {$devyc  * $::_scale}]
        set width  [expr {$width  * $::_scale}]
        set length [expr {$length * $::_scale}]
        
        set tr "$devxc:$devyc:ROT_0_REF_0"
        if {$flip == "i"} {
            set tr "$devxc:$devyc:ROT_0_REF_Y"
        }

        set gigenMgr [gigen_mgr_get_mgr]
        set mgigen [gigen_mgr_get_generator $gigenMgr $model]
        
        set useGen 1

        set sNetName [rawdevice_get_snet $rawdevice]
        set gNetName [rawdevice_get_gnet $rawdevice]
        set dNetName [rawdevice_get_dnet $rawdevice]
        set bNetName [rawdevice_get_bnet $rawdevice]

        set device [udm_utils_create_device $cell $devType $devName $length $width $tr $mgigen $useGen $sNetName $gNetName $dNetName $bNetName $model]
    }
    # end of create_device_from_raw_device
}
# end of namespace ::cln

#::cln::convert_to_lnf
