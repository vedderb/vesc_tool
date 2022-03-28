(define name-default "VESC_default.bin")
(define name-nolim "VESC_no_limits.bin")

(define pkg `(
            ("46_o_47"
                ("46" . ,name-default)
                ("46_no_limits" . ,name-nolim)
                ("46_33k" . "VESC_33k.bin")
                ("46_0005ohm" . "VESC_0005ohm.bin")
            )
            ("60"
                ("60" . ,name-default)
                ("60_no_limits" . ,name-nolim)
            )
))

; Print using loopforeach
(loopforeach hw pkg
    (progn
        (print (str-merge "Dir: " (car hw)))
        (loopforeach fw (cdr hw)
            (print (str-merge "  Target: " (car fw) " File: " (cdr fw))))
        (print " ")
))

; Same as above, but more functional approach for printing
(map (lambda (index)
    (let ( (dir (car (ix pkg index)))
           (fws (cdr (ix pkg index))) )
        (progn
            (print (str-merge "Dir: " dir))
            (map (lambda (x) (print (str-merge "  Target: " (car x) " File: " (cdr x)))) fws)
            (print " ")
))) (iota (length pkg)))
