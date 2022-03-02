(define name-default "VESC_default.bin")
(define name-nolim "VESC_no_limits.bin")

(define pkg `(
            ("46_o_47" . (
                ("46" . ,name-default)
                ("46_no_limits" . ,name-nolim)
                ("46_33k" . "VESC_33k.bin")
                ("46_0005ohm" . "VESC_0005ohm.bin")
            ))
            ("60" . (
                ("60" . ,name-default)
                ("60_no_limits" . ,name-nolim)
            ))
))

(defun print-fw (index)
    (let ( (dir (car (ix pkg index)))
           (fws (cdr (ix pkg index))) )
        (progn
            (print (list "Dir:" dir))
            (map (lambda (x) (print (list "  Target:" (car x) " File:" (cdr x)))) fws)
            (print " ")
)))

(map print-fw (iota (- (length pkg) 1)))
