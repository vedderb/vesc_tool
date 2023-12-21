(define spawncnt 0)
(define threads-now 0)

(defun downcounter (rate start)
    (if (= 0 start)
        (progn
            (define threads-now (- threads-now 1))
            1
        )
        (progn
            (yield (/ 1000000 rate))
            (downcounter rate (- start 1))
)))

(loopwhile t
    (progn
        (spawn 20 downcounter 500 15)
        (define threads-now (+ threads-now 1))
        (define spawncnt (+ spawncnt 1))
        (sleep 0.003)
))
