#t
#f
(if #f 1 0)
(if #t 1 0)
(if "67" 1 0)
(if (quote 5) 1 0)
(eq? 1 1)
(eq? (quote a) (quote a))
(eq? (list) (list))

