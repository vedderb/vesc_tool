(def logger-id -1) ; -1 means VESC Tool

(def val-a 0.0)
(def val-b 0.0)
(def v 0.0)
(def pi (* 2.0 (acos 0)))

(loopwhile-thd 100 t {
        (setq val-a (sin v))
        (setq val-b (cos v))
        (setq v (+ v 0.1))
        (if (> v (* 2.0 pi))
            (setq v (- v (* 2.0 pi)))
        )
        (sleep 0.05)
})

(log-config-field
    logger-id 0
    "val_a"
    "Value A"
    ""
    2 false false
)

(log-config-field
    logger-id 1
    "val_b"
    "Value B"
    ""
    2 false false
)

(log-config-field
    logger-id 2
    "v"
    "Angle"
    "rad"
    2 false false
)

(log-start logger-id 3 20 true false)

(loopwhile t {
        (log-send-f32 logger-id 0 val-a val-b v)
        (sleep 0.05)
})
