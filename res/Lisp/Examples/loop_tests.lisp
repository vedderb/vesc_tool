(print "== Performance for ==")

(define t0 (systime))
(define cnt 0)
(loopfor i 0 (< i 10000) (+ i 1) (define cnt (+ cnt i)))
(print (str-from-n cnt "For cnt: %d"))
(print (str-from-n (secs-since t0) "For time: %.2f s"))

(print "\n== Performance while ==")

(define t0 (systime))
(define cnt 0)
(define itr 0)
(loopwhile (< itr 10000) (progn (define cnt (+ cnt itr)) (define itr (+ itr 1))))
(print (str-from-n cnt "While cnt: %d"))
(print (str-from-n (secs-since t0) "While time: %.2f s"))

(print "\n== Performance range ==")

(define t0 (systime))
(define cnt 0)
(looprange i 0 10000 (define cnt (+ cnt i)))
(print (str-from-n cnt "Range cnt: %d"))
(print (str-from-n (secs-since t0) "Range time: %.2f s"))

(print "\n== For ==")

(loopfor i 0 (< i 5) (+ i 1)
    (progn
        (print i)
        (sleep 0.5)
))

(print "\n== While ==")

(define i 0)
(loopwhile (< i 5)
    (progn
        (print i)
        (sleep 0.5)
        (define i (+ i 1))
))

(print "\n== Range ==")

(looprange i 0 5
    (progn
        (print i)
        (sleep 0.5)
))

(print "\n== ForEach ==")

(loopforeach i '("AB" "C" "dE" "f")
    (print i)
)
