cat <<EOF >welsh-w.mf
%
% These parameters define a 10-point 'Welsh W' glyph.
% It has been designed to blend (somewhat) well with
% the Computer Modern Roman 10-point typeface.
%

font_size 10pt#;
 asc_height#:=250/36pt#;     % ascender height
 x_height#:=155/36pt#;       % x-height
 desc_depth#:=70/36pt#;      % descender depth
 u#:=18/36pt#;               % basic unit of width
 o#:=1/20pt#;                % curve adjustment below baseline

 boldness:=0.7;
 pentilt:=25;                % pen slantiness

mode_setup;

 em#:=17u#;      % M-width
 thick#:=2u#*boldness;  % width of vertical stems
 thin#:=0.3thick#;  % width of thin curved segments

define_pixels(em,x_height,asc_height,desc_depth,u,o);
define_blacker_pixels(thin,thick);


% Create the pens we'll use for all our work
clear_pen_memory;
pickup pencircle xscaled thick yscaled thin rotated pentilt;
oval.nib := savepen;
pickup pensquare xscaled thick yscaled thin rotated pentilt;
rect.nib := savepen;


beginchar("w",9.5u#,asc_height#,0); "Welsh w";
 pickup oval.nib;
 rt x1=w-u; top y1=x_height;
 lft x3=u; y3=.5x_height;
 x2=.5[x1,x3]; bot y2=-o;
 pickup rect.nib; top y4=h;
 z4=z1+whatever * dir (90+pentilt);
 z3d=(x3-x2,1.5*y3-y2);
 path ovpath;
 ovpath = z1..{-dir pentilt}z2..tension 2.0..{dir (90+pentilt)}z3 
          ..{dir pentilt}z4;
 pickup oval.nib; draw ovpath;
 pickup rect.nib; draw point 3 of ovpath;
endchar;


font_slant           0;
font_normal_space    1/3em#;
font_normal_stretch  1/9em#;
font_normal_shrink   1/12em#;
font_x_height        x_height#;
font_quad            em#;
font_extra_space     0;
EOF

cat <<EOF >welsh-w.tex
\documentclass{article}
\newfont{\welsh}{welsh-w scaled 4000}
\newfont{\rmbig}{cmr10 scaled 4000}
\newcommand{\w}{\welsh w}
\newcommand{\s}{f}
\renewcommand{\r}{r}
\newcommand{\n}{\rlap{\d{ }}n\llap{\d{ }}}
\begin{document}
\rmbig\baselineskip 48pt

{\w}n. k\.y{\r}ch{\w}n loeg\.y{\r} ha{\w}{\s}{\s}af \.y{\w}
in \.ymbo{\r}th {\n}\d{i} \d{a}\d{l}\d{l}\d{\w}{\n} \.yno. \.Yn

\end{document}
EOF

mf '\mode=localfont; mag=4; \input welsh-w \end'
gftopk welsh-w.2400gf welsh-w.2400pk
pdflatex welsh-w
