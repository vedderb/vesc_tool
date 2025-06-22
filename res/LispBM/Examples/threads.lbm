(defun counter (v rate start) {
        (set v start)
        (sleep (/ 1.0 rate))
        (counter v rate (+ start 1))
})

; Start lots of threads where each thread has
; 25 symbols of stack.
(looprange i 1 26 {
        (var cnt-sym (read (str-from-n i "cnt%02d")))
        (eval `(def ,cnt-sym 0))
        (eval `(spawn 25 ,counter ,(quote cnt-sym) ,(* i 10) 0))
})
