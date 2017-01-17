(define builtin-define define)
(define define
  (macro args (cons (quote builtin-define) args)))

