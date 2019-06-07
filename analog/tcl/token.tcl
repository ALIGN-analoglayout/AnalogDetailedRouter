#
# Author: Nikolai Ryzhenko 10984646 nikolai.v.ryzhenko@intel.com
# Date: November 8, 2017
# 

package require Itcl

namespace eval ::tkn {

    set classesToDelete [list "TokenManager" "Token"]
    foreach classToDelete $classesToDelete {
        foreach classid [::itcl::find classes] {
            set index [string first $classToDelete $classid]
            if {$index != 0} { continue }
            if {[::itcl::find classes $classid] == ""} { continue }
            puts "Deleting class $classid"
            delete class $classid
        }
    }
    
    ###########################################################################################################
    #
    # class Token
    #
    ###########################################################################################################

    ::itcl::class Token {

        # constructor
        constructor {args} {}
        
        # destructor
        destructor {}

        private variable _isroot 0
        private variable _name ""
        private variable _originalLine ""
        private variable _pars [list]
        private variable _vals [list]
        private variable _tokens [list]

        public method mark_as_root {} { set _isroot 1}
        
        public method set_name {v} { set _name $v }
        public method set_original_line {v} { set _originalLine $v }
        
        public method add_par_val {par val} {
            lappend _pars $par;
            lappend _vals $val;
        }

        public method add_token {token} { lappend _tokens $token }
        
        public method get_name {} { return $_name }
        public method get_pars {} { return $_pars }
        public method get_vals {} { return $_vals }

        public method get_original_line {} { return $_originalLine }

        public method get_tokens {{tokenname ""} {depth 0} {minNum -1} {maxNum -1}}
        public method get_single_token {tokenname}
        
        public method parse_par_val_str {str}
        public method print_tokens {{out ""} {prefix ""}}
        public method get_value {parname {mandatory 1} {mustbeset 1} {defaultvalue ""}}
        public method get_value_as_list {parname {delim ","}}
        public method get_value_of_a_single_subtoken {tokenname parname}
        public method is_defined {parname}
        public method add_new_token {{name ""}}
    }
    # end of class Token

    ::itcl::body Token::destructor {} {

        foreach token $_tokens {
            #puts "Token::destructor: deleting token=$token"
            #if {[find objects $token] != ""} {
            #    delete object $token
            #}
            ::itcl::delete object $token
        }
    }
    # end of Token::destructor

    # parse "par=val"
    ::itcl::body Token::parse_par_val_str {strtoken} {

        set index [string first "=" $strtoken]
        if {$index < 0} {
            #error "can't parse expression '$strtoken' as a parameter-value pair"
            set parameter $strtoken
            set value ""
        } else {
            set parameter [string range $strtoken 0 [expr {$index-1}]]
            set value [string range $strtoken [expr {$index+1}] end]
        }
        
        if {[string trim $parameter] == ""} { error "empty parameter" }
        #if {[string trim $value] == ""} { error "empty value" }
        
        add_par_val $parameter $value        
    }
    # end of Token::parse_par_val_str

    ::itcl::body Token::print_tokens {{out ""} {prefix ""}} {

        if {$_isroot} {
            foreach token $_tokens {
                $token print_tokens $out $prefix
            }
            return
        }

        set str $prefix
        append str $_name
        
        set numpars [llength $_pars]
        for {set i 0} {$i < $numpars} {incr i} {
            set par [lindex $_pars $i]
            set val [lindex $_vals $i]
            append str " $par=$val"
        }
        
        if {[llength $_tokens] != 0} {
            append str " \{"
        }
        
        if {$out != ""} {
            puts $out $str
        } else {
            puts $str
        }
        
        foreach token $_tokens {
            $token print_tokens $out "$prefix  "
        }
        
        if {[llength $_tokens] != 0} {
            if {$out != ""} {
                puts $out "${prefix}\}"
            } else {
                puts "${prefix}\}"
            }
        }
    }
    # end of Token::print_tokens

    ::itcl::body Token::get_tokens {{tokenname ""} {depth 0} {minNum -1} {maxNum -1}} {

        set tokens [list]
       
        if {$tokenname == ""} { 
            
            set tokens $_tokens
            
        } else {

            foreach token $_tokens {
                
                if {[$token get_name] == $tokenname} {
                    lappend tokens $token
                }
                
                if {$depth > 0} {
                    set deeptokens [$token get_tokens $tokenname [expr {$depth-1}]]
                    foreach deeptoken $deeptokens {
                        lappend tokens $deeptoken
                    }
                }
            }
        }

        set numtokens [llength $tokens]

        if {$minNum >= 0 && $numtokens < $minNum} { error "unexpected number=${numtokens} of token tokenname=${tokenname}; minNum=${minNum}; owner name=[get_name]" }
        if {$maxNum >= 0 && $numtokens > $maxNum} { error "unexpected number=${numtokens} of token tokenname=${tokenname}; maxNum=${maxNum}; owner name=[get_name]" }

        return $tokens
    }
    # end of Token::get_tokens

    ::itcl::body Token::get_single_token {tokenname} {

        set tokens [get_tokens $tokenname]
        if {[llength $tokens] != 1} {
            $token print_tokens
            error "bad token"
        }

        return [lindex $tokens 0]
    }
    # end of Token::get_single_token

    ::itcl::body Token::get_value {parname {mandatory 1} {mustbeset 1} {defaultvalue ""}} {

        set numpars [llength $_pars]
        set numvals [llength $_vals]

        if {$numpars != $numvals} {
            error  "Token::get_value: number of parameters and number of values don't match: $numpars != $numvals"
        }

        for {set i 0} {$i < $numpars} {incr i} {
            
            set par [lindex $_pars $i]
            if {$par != $parname} { continue }

            set val [lindex $_vals $i]
            if {$mustbeset && $val == ""} {
                error "Token::get_value: token $_name: parameter $parname: value is not set"
            }
            return $val
        }

        if {$mandatory} {
            error "Token::get_value: token $_name: parameter $parname: parameter is missed"
        }

        return $defaultvalue
    }
    # end of Token::get_value

    ::itcl::body Token::get_value_as_list {parname {delim ","}} {
        
        return [split [get_value $parname] $delim]
    }
    # end of Token::get_value_as_list

    ::itcl::body Token::get_value_of_a_single_subtoken {tokenname parname} {

        set subtokens [get_tokens $tokenname]
        
        if {[llength $subtokens] != 1} {
            print_tokens
            error "unexpected token"
        }
        
        set subtoken [lindex $subtokens 0]
        
        return [$subtoken get_value $parname]
    }
    # end of Token::get_value_of_a_single_subtoken

    #
    ::itcl::body Token::is_defined {parname} {

        set numpars [llength $_pars]

        for {set i 0} {$i < $numpars} {incr i} {
            
            set par [lindex $_pars $i]
            if {$par == $parname} { return 1 }
        }
        
        return 0
    }
    # end of Token::is_defined

    #
    ::itcl::body Token::add_new_token {{name ""}} {

        set token [::tkn::create_token]

        if {$name != ""} {
            $token set_name $name
        }

        add_token $token

        return $token
    }
    # end of Token::add_new_token

    ###########################################################################################################
    #
    # class TokenManager
    #
    ###########################################################################################################
    
    ::itcl::class TokenManager {

        # constructor
        constructor {} {}
        
        # destructor
        destructor {}

        private variable _roottoken ""

        public method get_root_token {} { return $_roottoken }
        
        public method parse_token_file {filename}

        private method parse_file_line_ {file tokenowner}
    }
    # end of class TokenManager

    ::itcl::body TokenManager::destructor {} {

        if {$_roottoken != ""} {
            #puts "TokenManager::destructor: deleting _roottoken=$_roottoken"
            if {[::itcl::find objects $_roottoken] != ""} {
                ::itcl::delete object $_roottoken
            }
        }
    }
    # end of TokenManager::destructor

    ::itcl::body TokenManager::constructor {} {

        set _roottoken [::tkn::create_token]
        
        $_roottoken mark_as_root        
    }
    # end of TokenManager::constructor

    ::itcl::body TokenManager::parse_token_file {filename} {

        #puts "parse token file $filename"

        set file [open $filename "r"]
        if {$file == ""} {
            error "TokenManager::parse_token_file: can't open file $filename for reading"
        }

        if {$_roottoken == ""} {
            error "Root token is null."
        }

        while {1} {
            set ok [parse_file_line_ $file $_roottoken]
            if {$ok == -1} {
                break
            }
        }

        close $file
    }
    # end of TokenManager::parse_token_file

    ::itcl::body TokenManager::parse_file_line_ {file tokenowner} {

        if {[gets $file line] < 0} { return -1 }

        set initialline $line

        set line [string trim $line]
        set index [string first "\#" $line]
        if {$index == 0} { return 1 }
        #if {$index > 0} {
        #	set line [string range $line 0 [expr {$index-1}]]
        #}
        set line [string trim $line]
        if {$line == ""} { return 1 }
        
        if {[scan $line "%s" tokenname] != 1} {
            error "it's impossible!"
        }

        if {$tokenname == "\}"} {
            if {$tokenname != $line} {
                error "parse error: bad line: $initialline"
            }
            return 0
        }

        set tokenobj [::tkn::create_token]
        $tokenowner add_token $tokenobj
        $tokenobj set_name $tokenname
        $tokenobj set_original_line $line

        set needrecursion 0
        set index [string last "\{" $line]
        if {$index > 0} {
            set needrecursion 1
            set line [string range $line 0 [expr {$index-1}]]
            set line [string trim $line]
        }
        
        set numstrtokens [llength $line]
        
        for {set i 1} {$i < $numstrtokens} {incr i} {

            set strtoken [lindex $line $i]
            
            #if {[expr {$i+1}] == $numstrtokens && $strtoken == "\{"} {
            #	set needrecursion 1
            #	break
            #}

            $tokenobj parse_par_val_str $strtoken
        }

        if {!$needrecursion} { return 1 }

        while {1} {
            set ok [parse_file_line_ $file $tokenobj]
            if {$ok == -1} { return -1 }
            if {$ok == 0} { return 1 }
        }
    }
    # end of TokenManager::parse_file_line_

    ###########################################################################################################
    #
    # API procedures of this namespace
    #
    ###########################################################################################################

    #
    proc create_token {{name ""}} {

        set token [::tkn::Token \#auto]

        $token set_name $name

        return ::tkn::${token}
    }
    # end of create_token

    #
    proc create_token_manager {} {

        set TokenManager [::tkn::TokenManager \#auto]

        return ::tkn::${TokenManager}
    }
    # end of create_token_manager

    #
    proc delete_token_manager {tokenmanager} {

        #puts "delete_token_manager: deleting tokenmanager=$tokenmanager"
        if {[::itcl::find objects $tokenmanager] != ""} {
            ::itcl::delete object $tokenmanager
        }
    }
    # end of delete_token_manager

}
# end of namespace ::tkn
