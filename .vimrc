" This commands will executed by vim, if global .vimrc containing follwing
" settings:
"    set exrc
"    set secure
"
"          'exrc' allows loading local executing local rc files.
"          'secure' disallows the use of :autocmd, shell and write
"          commands in local .vimrc files.
"
let s:cur_dir = expand('%:p:h')
let s:include_folders = [ ''
			\ , '"' . s:cur_dir . '/include"'
			\ , '"' . s:cur_dir . '/src"'
			\ ]  " Empty first entry(!)

" Hm, maybe not allowed
"augroup filetypedetect
"	au! BufRead,BufNewFile *.h         setfiletype c
"augroup END

" Settings for ALE
let g:ale_c_cc_header_exts = ['h']
let g:ale_cpp_cc_header_exts = ['hpp']
let g:ale_c_cc_options = '-std=c18 -Wall'.join(s:include_folders, ' -I')
let g:ale_cpp_clang_options = '-std=c++17 -Wall -I'.join(s:include_folders, ' -I')

" Needed to find compile_commands.json in one of the (default) build folders
" or user defined one. (for clang-check)
" Maybe this leads into problems if multiple files exists but not the latest
" version is seleted?!
let s:build_dirs = 
			\ [ s:cur_dir . '/build'
			\ , s:cur_dir . '/debug'
			\ , s:cur_dir . '/debug_with_tests' 
			\ , s:cur_dir . findfile('compile_commands.json' , '**1')
			\ ]
let g:ale_c_build_dir_names = s:build_dirs
let g:ale_cpp_build_dir_names = s:build_dirs
