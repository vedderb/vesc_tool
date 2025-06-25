(def name-default "VESC_default.bin")
(def name-nolim "VESC_no_limits.bin")

(def pkg `(
        ("46_o_47"
            ("46" ,name-default)
            ("46_no_limits" ,name-nolim)
            ("46_33k" "VESC_33k.bin")
            ("46_0005ohm" "VESC_0005ohm.bin")
        )
        ("60"
            ("60" ,name-default)
            ("60_no_limits" ,name-nolim)
        )
))

; Pad string s to length n with character ch
(defun pad-str (s n ch)
    (if (>= (str-len s) n)
        s
        (pad-str (str-merge s ch) n ch)
))

(loopforeach hw pkg {
        (print (str-merge "Dir: " (first hw)))
        
        (loopforeach fw (rest hw)
            (print (str-merge "  Target: " (pad-str (first fw) 12 " ") " File: " (second fw)))
        )
        
        (print " ")
})
