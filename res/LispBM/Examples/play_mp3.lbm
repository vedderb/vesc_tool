; This examples requires running Mp3Stream.qml in VESC Tool and that
; the connection to VESC Tool is sufficiently fast (e.g. USB)

(def wait-pid -1)

(defun event-handler ()
    (loopwhile t
        (recv
            ((event-data-rx . (? data)) (send wait-pid data))
            (_ nil)
)))

(event-register-handler (spawn event-handler))
(event-enable 'event-data-rx)

(defun get-samples () {
        (setq wait-pid (self))
        (send-data '(1))
        (recv-to 0.001
            (timeout {
                    (recv-to 0.1
                        (timeout (print "Timeout"))
                        ((? data) data)
                    )
            })
            ((? data) data)
        )
})

(defun test-speed () {
        (var start (systime))
        (get-samples)
        (secs-since start)
})

(def chunk-size 500)

(def buf1 (bufcreate chunk-size))
(def buf2 (bufcreate chunk-size))
(def buf3 (bufcreate chunk-size))

(defun play-samples (buf) {
        (var samp (get-samples))
        (def num-samp (bufget-u32 samp 0))
        (def f-samp (bufget-u32 samp 4))
        (bufcpy buf 0 samp 8 chunk-size)
        (free samp)
        
        (foc-play-samples buf f-samp 0.3)
})

(loopwhile t {
        (play-samples buf1)
        (play-samples buf2)
        (play-samples buf3)
})
