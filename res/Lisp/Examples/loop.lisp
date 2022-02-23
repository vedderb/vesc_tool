(define rate 50) ; Update rate in hz

(defun f ()
    (progn
        ; Put your code here

        (yield (/ 1000000 rate))
        (f)
))

(f)
