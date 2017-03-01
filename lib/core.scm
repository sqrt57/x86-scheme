(define (memq obj list)
  (if (pair? list)
      (if (eq? (car list) obj)
          list
          (memq obj (cdr list)))
      #f))

(define *features* (quote (x86-scheme)))

(define (add-feature! feature)
  (if (memq feature *features*)
    (begin)
    (set! *features* (cons feature *features*))))

(define (has-feature? feature)
  (if (memq feature *features*) #t #f))
