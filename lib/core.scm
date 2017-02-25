(define (memq obj list)
  (if (pair? list)
      (if (eq? (car list) obj)
          list
          (memq obj (cdr list)))
      #f))
