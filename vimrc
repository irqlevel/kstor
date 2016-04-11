function! SetupEnvironment()
  let l:path = expand('%:p')
  if l:path =~ '/root/linux'
    setlocal tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab
  elseif l:path =~ '/root/nkfs'
    setlocal tabstop=8 softtabstop=8 shiftwidth=8 noexpandtab
  elseif l:path =~ '/root/kcpp'
    setlocal softtabstop=4 shiftwidth=4 expandtab
  endif
endfunction
autocmd! BufReadPost,BufNewFile * call SetupEnvironment()
:highlight ExtraWhitespace ctermbg=red guibg=red
autocmd Syntax * syn match ExtraWhitespace /\s\+$\| \+\ze\t/
:2mat ErrorMsg '\%81v.'
