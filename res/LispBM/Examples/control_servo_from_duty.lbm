(def servo-out 0.5)

(defun abs (x) (if (> x 0) x (- x)))

(defun update-servo () {
        (def servo-out (+ servo-out (* (get-duty) 0.2)))
        (if (> servo-out 1.0) (def servo-out 1.0) nil)
        (if (< servo-out 0) (def servo-out 0) nil)
        (set-servo servo-out)
})

(def itcnt 0)

(loopwhile t {
        (def itcnt (+ itcnt 1))
        (if (> (abs (get-duty)) 0.005) (update-servo) nil)
        (sleep 0.01)
})
