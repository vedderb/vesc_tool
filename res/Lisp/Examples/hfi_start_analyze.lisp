(def samples 30) ; 0 - 300 A

; Start with reasonable valies for something like a QS165
(conf-set 'foc-hfi-amb-mode 2)
(conf-set 'foc-hfi-start-samples 150)
(conf-set 'foc-hfi-voltage-start 30.0)
(conf-set 'foc-hfi-voltage-run 30.0)
(conf-set 'foc-hfi-voltage-max 30.0)

(plot-init "Current" "Response")
(plot-add-graph "Diff abs")
(plot-add-graph "I-pos")
(plot-add-graph "I-neg")

(looprange i 0 samples {
        (var current (* i 10.0))
        (conf-set 'foc-hfi-amb-current current)
        (set-current 5.0)
        (sleep 0.3)
        (set-current 0.0)

        (var (i1 i2 diff) (get-hfi-res))

        (plot-set-graph 0)
        (plot-send-points current (* diff 100.0))
        (plot-set-graph 1)
        (plot-send-points current i1)
        (plot-set-graph 2)
        (plot-send-points current i2)
})
