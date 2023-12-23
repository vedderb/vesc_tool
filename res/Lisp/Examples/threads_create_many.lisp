(def spawncnt 0)
(def threads-now 0)

(defun downcnt (rate start) {
        (loopwhile (> start 0) {
                (setq start (- start 1))
                (sleep (/ 1.0 rate))
        })
        
        (atomic (setq threads-now (- threads-now 1)))
})

(loopwhile t {
        (spawn 30 downcnt 500 15)
        (atomic (setq threads-now (+ threads-now 1)))
        (setq spawncnt (+ spawncnt 1))
        (sleep 0.003)
})
