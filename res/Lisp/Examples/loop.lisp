(defun f ()
    (progn
        ; Put your code here

        (sleep 0.02) ; 50 Hz
        (f)
))

(f)
