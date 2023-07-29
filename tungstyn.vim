" vim syntax file for Tungstyn

setlocal ai
syn iskeyword @,48-57,?,!,+,-,*,/,%,=,_,&,|,<,>

" commands
syn keyword wCommand if break continue return while do for echo echoln read readln + - * / % = != < <= > >= & \| let! set! swap! list new-list range map cmd refcount clone int string float
hi link wCommand Statement

syn match wVar /$\k*/
hi link wVar Identifier

syn keyword wNull null
hi link wNull Constant

syn match wNumber /\d\+\(\.\d*\)\?/
hi link wNumber Number

syn match wComment /#.*$/
hi link wComment Comment

syn region wString start=/"/ skip=/\\"/ end=/"/
hi link wString String

