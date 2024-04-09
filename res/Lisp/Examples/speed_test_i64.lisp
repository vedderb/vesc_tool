(defun tak (x y z)
    (if (not (< y x))
        z
        (tak
            (tak (- x 1) y z)
            (tak (- y 1) z x)
            (tak (- z 1) x y)
)))

(def start (systime))
(def takres (tak 16 12 6))
(def time (secs-since start))
(print (list "Time i28:" time))

(def start (systime))
(def takres (tak 16i64 12i64 6i64))
(def time (secs-since start))
(print (list "Time i64:" time))
