; Start ICU-timer at 1 MHz and measure the high part of the pulse
(icu-start 1000000 1)

; Callback counter
(def cb-cnt 0)

(defun proc-icu (width period) {
        (def icu-w width)
        (def icu-p period)
        (def cb-cnt (+ cb-cnt 1))
})

(defun event-handler ()
    (loopwhile t
        (recv
            ((event-icu-period . ((? width) . (? period))) (proc-icu width period))
            (_ nil)
)))

(event-register-handler (spawn event-handler))
(event-enable 'event-icu-period)
