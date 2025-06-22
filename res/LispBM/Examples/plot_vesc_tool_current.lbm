; This makes a plot on the experiment plot tab.

(plot-init "Sample" "Current (A)")
(plot-add-graph "ia")
(plot-add-graph "ib")
(plot-add-graph "ic")
(plot-add-graph "i_motor")

(looprange i 0 200 {
        (foc-openloop 50 50)
        (plot-set-graph 0)
        (plot-send-points i (raw-adc-current 1 1 0))
        (plot-set-graph 1)
        (plot-send-points i (raw-adc-current 1 2 0))
        (plot-set-graph 2)
        (plot-send-points i (raw-adc-current 1 3 0))
        (plot-set-graph 3)
        (plot-send-points i (get-current))
        (sleep 0.01)
})

(set-current 0)
