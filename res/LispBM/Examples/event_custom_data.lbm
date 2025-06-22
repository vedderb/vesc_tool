(defun proc-data (data)
    (print data)
)

(defun event-handler ()
    (loopwhile t
        (recv
            ((event-data-rx . (? data)) (proc-data data))
            (_ nil) ; Ignore other events
)))

; Spawn the event handler thread and pass the ID it returns to C
(event-register-handler (spawn event-handler))

; Enable the custom app data event
(event-enable 'event-data-rx)
