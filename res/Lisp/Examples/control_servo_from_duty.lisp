(define servo-out 0.5)

(defun abs (x) (if (> x 0) x (- x)))

(defun update-servo ()
  (progn
    (define servo-out (+ servo-out (* (get-duty) 0.2)))
    (if (> servo-out 1.0) (define servo-out 1.0) nil)
    (if (< servo-out 0) (define servo-out 0) nil)
    (set-servo servo-out)
))

(define itcnt 0)

(loopwhile t
   (progn
     (define itcnt (+ itcnt 1))
     (if (> (abs (get-duty)) 0.005) (update-servo) nil)
     (sleep 0.01)
))
