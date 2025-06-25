; Module: VESC Express
; Connections
; Pin 20: 45x GRBW

; Helper function to make a list populated with val
(defun mklist (len val) (map (fn (x) val) (range len)))

; Variables
(def leds-s1 45)
(def strip1 (rgbled-buffer leds-s1 2 1))
(def colors1 (mklist leds-s1 0))
(def leds-on t)
(def brightness 1.0)

(rgbled-init 20)

; Thread that fades all LEDs towards 0
(loopwhile-thd 100 t {
        (atomic (setq colors1 (color-scale colors1 0.8)))
        (rgbled-color strip1 0 colors1 brightness)
        (rgbled-update strip1)
        (sleep 0.03)
})

; Thread that fades the brightness when switching on and off the LEDs
(loopwhile-thd 100 t {
        (if leds-on
            (if (< brightness 1.0)
                (setq brightness (+ brightness 0.02))
            )
            (if (> brightness 0.0)
                (setq brightness (- brightness 0.02))
            )
        )

        (sleep 0.02)
})

; Add color to one of the colors
; Color is the list (r g b)
(defun add-color (led color)
    (atomic
        (setix colors1 led (color-add
                (ix colors1 led)
                (apply color-make color)
))))

; Animation 1
(loopwhile-thd 100 t {
        (looprange i 0 leds-s1 {
                (add-color i '(255 0 0 50))
                (add-color (- leds-s1 i) '(0 0 255))
                (sleep 0.04)
        })
})

; Animation 2
; Random green blinks
(loopwhile-thd 100 t {
        (add-color (mod (rand) leds-s1) '(0 255 0))
        (sleep 0.2)
})

; Switch LEDs on and of every few seconds
(loopwhile-thd 100 t {
        (setq leds-on t)
        (sleep 5)
        (setq leds-on nil)
        (sleep 2)
})
