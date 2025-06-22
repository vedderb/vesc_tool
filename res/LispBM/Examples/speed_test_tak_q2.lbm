; These functions are taken from here
; http://www.ulisp.com/show?1EO1

(defun tak (x y z)
    (if (not (< y x))
        z
        (tak
            (tak (- x 1) y z)
            (tak (- y 1) z x)
            (tak (- z 1) x y)
)))

(defun q2 (x y)
    (if (or (< x 1) (< y 1))
        1
        (+ (q2 (- x (q2 (- x 1) y)) y)
            (q2 x (- y (q2 x (- y 1))))
)))

(def start (systime))
(def takres (tak 18 12 6))
(def time (secs-since start))
(print (list "Time tak:" time))

; Q2 takes more stack, so use thread with extra stack
(wait (spawn 1024 (fn () {
                (def start (systime))
                (def q2res (q2 7 8))
                (def time (secs-since start))
                (print (list "Time q2:" time))
})))

(def start (systime))
(gc)(gc)(gc)(gc)(gc)
(gc)(gc)(gc)(gc)(gc)
(def time (secs-since start))
(print (list "Time gc:" (/ time 10.0)))

; Result 2023-02-23 on ESP32-C3
;("Time tak:" {2.515000})
;("Time q2:" {6.548000})
;("Time gc:" {0.000500})
