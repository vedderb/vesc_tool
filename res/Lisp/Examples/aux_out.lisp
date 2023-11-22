(defun set-auxtime (port state time) {
        (set-aux port state)
        (sleep time)
})

(loopwhile t {
        (set-auxtime 1 1 1.5)
        (set-auxtime 1 0 0.5)
        (set-auxtime 2 1 1.5)
        (set-auxtime 2 0 0.5)
        (sleep 1.0)
})

