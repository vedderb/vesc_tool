(defun repeat (f times)
    (if (<= times 1)
        (f)
        { (f) (repeat f (- times 1)) }
))

(defun run-gc () {
        (def a1 0) (def a2 0) (def a3 0) (def a4 0) (def a5 0)
        (def a6 0) (def a7 0) (def a8 0) (def a9 0) (def a10 0)
        (def b1 0) (def b2 0) (def b3 0) (def b4 0) (def b5 0)
        (def b6 0) (def b7 0) (def b8 0) (def b9 0) (def b10 0)
        (def c1 0) (def c2 0) (def c3 0) (def c4 0) (def c5 0)
        (def c6 0) (def c7 0) (def c8 0) (def c9 0) (def c10 0)
        
        (def start (systime))
        (repeat 'gc 25)
        (def res (secs-since start))
        (print (list "GC-time (ms):" (* (/ res 25) 1000)))
})

(looprange i 0 100 (run-gc))
(print "Done!")
