#! /usr/intel/bin/tclsh8.5

namespace eval ::cln {

    #
    proc select_one_lgf_file_per_family {dir} {
        
        set files [exec ls $dir]

        array set cellsPerKey {}

        foreach filename $files {
            
            set index [string first ".lgf" $filename]
            if {$index < 0} { continue }

            set cellfamily [string range $filename 0 8]
            set celltype   [string range $filename 9 11]
            set rowheight  [string range $filename 12 12]
            set cellsize   [string range $filename 13 16]

            #puts "$filename $cellfamily $celltype $rowheight $cellsize"

            if {$rowheight != "n" && $rowheight != "d"} { continue }

            set key "$cellfamily $rowheight"

            if {[llength [array names  cellsPerKey-exact $key]] == 0} {
                set cellsPerKey($key) [list]
            }

            lappend cellsPerKey($key) $filename
        }

        foreach {key value} [array get cellsPerKey] {

            set filenames $value
            
            # one cell per family per size
            for {set i 0} {$i < 1} {incr i} {
                set filename [lindex $filenames $i]
                puts $filename
            }
        }

        array unset cellsPerKey
    }
    # end of select_one_lgf_file_per_family
}
# end of namespace ::cln

set i 0
if {$i < [llength $argv]} { set dir [lindex $argv $i] ; incr i } else { error "no directory with LGF files" }

::cln::select_one_lgf_file_per_family $dir
