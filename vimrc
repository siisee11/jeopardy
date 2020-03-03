"=========Vundel============

set nocompatible              " be iMproved, required
filetype off                  " required

" set the runtime path to include Vundle and initialize
set rtp+=~/.vim/bundle/Vundle.vim
call vundle#begin()
" alternatively, pass a path where Vundle should install plugins
"call vundle#begin('~/some/path/here')

" let Vundle manage Vundle, required
Plugin 'VundleVim/Vundle.vim'

Plugin 'tpope/vim-fugitive'
Plugin 'git://git.wincent.com/command-t.git'
Plugin 'rstacruz/sparkup', {'rtp': 'vim/'}
Plugin 'scrooloose/nerdtree'
Plugin 'bling/vim-airline' 
Plugin 'tpope/vim-abolish'
Plugin 'Valloric/YouCompleteMe'
Plugin 'Lokaltog/vim-easymotion'
Plugin 'fatih/vim-go'
Plugin 'ronakg/quickr-cscope.vim'
Plugin 'taglist.vim'

" All of your Plugins must be added before the following line
call vundle#end()            " required
filetype plugin indent on    " required
"===========Vundle!=============



"============주석==========
func! CmtOn()    "주석 on
exe "'<,'>norm i//"
endfunc
func! CmtOff()    "주석 off
	exe "'<,'>norm 2x"
	endfunc
	vmap <c-j> <esc>:call CmtOn() <cr>
	vmap <c-x> <esc>:call CmtOff() <cr>
	nmap <c-j> <esc>v:call CmtOn() <cr>
	nmap <c-x> <esc>v:call CmtOff() <cr>
"================================================================

set ruler
set wrap
set number "line number (<-> set nonumber)
set tabstop=4 " 탭문자는 4컬럼 크기로 보여주기
set shiftwidth=4 " 문단이나 라인을 쉬프트할 때 4컬럼씩 하기
set autoindent " 자동 들여쓰기
syntax on " 적절히 Syntax에 따라 하일라이팅 해주기
set cindent " C 언어 자동 들여쓰기
set showmatch       " 매치되는 괄호의 반대쪽을 보여줌
set title           " 타이틀바에 현재 편집중인 파일을 표시
set smartindent " 좀더 똑똑한 들여쓰기를 위한 옵션이다.
set ignorecase
set incsearch
set showcmd
set nowrap
set laststatus=2
set autoindent " 자동으로 들여쓰기를 한다.
set ts=4 "Tab space
set sw=4
set hlsearch "highlight search
set cursorline
syntax enable
highlight Comment term=bold cterm=bold ctermfg=2
 "이건 주석의 색깔을 지정. 2는 초록

map ,1 :b!1<CR>
map ,2 :b!2<CR>
map ,3 :b!3<CR>
map ,4 :b!4<CR>
map ,5 :b!5<CR>
map ,6 :b!6<CR>
map ,7 :b!7<CR>
map ,8 :b!8<CR>
map ,9 :b!9<CR>
map ,0 :b!0<CR>
"keybind NERDTREE Toggle button to F7
nmap <F7> :NERDTreeToggle<CR>
nmap <c-c> <esc>:w<CR><esc><c-z>


"=====================ctags 설정 ============
set tags=./tags ",/usr/src/tags
set tags=/home/siisee11/git/linux-PMDFC/tags


let g:easytags_async=1
let g:easytags_auto_highlight = 0
let g:easytags_include_members = 1
let g:easytags_dynamic_files = 1


"==================== cscope 설정 ========
function! LoadCscope()
  let db = findfile("cscope.out", ".;")
  if (!empty(db))
    let path = strpart(db, 0, match(db, "/cscope.out$"))
    set nocscopeverbose " suppress 'duplicate connection' error
    exe "cs add " . db . " " . path
    set cscopeverbose
  " else add the database pointed to by environment variable 
  elseif $CSCOPE_DB != "" 
    cs add $CSCOPE_DB
  endif
endfunction
au BufEnter /* call LoadCscope()

"================== Tlist 설정 ==========
nnoremap <silent> <F8> :Tlist<CR>
let Tlist_Use_Right_Window = 1

"Open file at previous reading point
au BufReadPost *
\ if line("'\"") > 0 && line("'\"") <= line("$") |
\ exe "norm g`\"" |
\ endif
