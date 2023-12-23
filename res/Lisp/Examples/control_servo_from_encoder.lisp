(loopwhile t {
        (def encval (get-encoder)) ; So that lisp_stats shows the value
        (set-servo (/ (- 360.0 encval) 360.0))
        (sleep 0.02)
})
