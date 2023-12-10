/* 
 *    This a little on the complex side, but I couldn't think of any 
 *    other way to do it. I was getting fed up with having to rewrite 
 *    my asm code every time I altered the layout of a C struct, but I 
 *    couldn't figure out any way to get the asm stuff to read and 
 *    understand the C headers. So I made this program. It includes 
 *    allegro.h so it knows about everything the C code uses, and when 
 *    run it spews out a bunch of #defines containing information about 
 *    structure sizes which the asm code can refer to.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef DJGPP
#include <dpmi.h>
#endif

#include "allegro.h"
#include "internal.h"


int main(int argc, char *argv[])
{
   FILE *f;
   int x, y;

   if (argc < 2) {
      printf("Usage: asmdef <output file>\n");
      return 1;
   }

   printf("writing structure offsets into %s...\n", argv[1]);

   f = fopen(argv[1], "w");

   fprintf(f, "/* automatically generated structure offsets for use by asm code */\n\n");

   #ifdef ALLEGRO_COLOR16
      fprintf(f, "#ifndef ALLEGRO_COLOR16\n");
      fprintf(f, "#define ALLEGRO_COLOR16\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR24
      fprintf(f, "#ifndef ALLEGRO_COLOR24\n");
      fprintf(f, "#define ALLEGRO_COLOR24\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   #ifdef ALLEGRO_COLOR32
      fprintf(f, "#ifndef ALLEGRO_COLOR32\n");
      fprintf(f, "#define ALLEGRO_COLOR32\n");
      fprintf(f, "#endif\n");
      fprintf(f, "\n");
   #endif

   fprintf(f, "#define BMP_W                 %ld\n",  offsetof(BITMAP, w));
   fprintf(f, "#define BMP_H                 %ld\n",  offsetof(BITMAP, h));
   fprintf(f, "#define BMP_CLIP              %ld\n",  offsetof(BITMAP, clip));
   fprintf(f, "#define BMP_CL                %ld\n",  offsetof(BITMAP, cl));
   fprintf(f, "#define BMP_CR                %ld\n",  offsetof(BITMAP, cr));
   fprintf(f, "#define BMP_CT                %ld\n",  offsetof(BITMAP, ct));
   fprintf(f, "#define BMP_CB                %ld\n",  offsetof(BITMAP, cb));
   fprintf(f, "#define BMP_VTABLE            %ld\n",  offsetof(BITMAP, vtable));
   fprintf(f, "#define BMP_WBANK             %ld\n",  offsetof(BITMAP, write_bank));
   fprintf(f, "#define BMP_RBANK             %ld\n",  offsetof(BITMAP, read_bank));
   fprintf(f, "#define BMP_DAT               %ld\n",  offsetof(BITMAP, dat));
   fprintf(f, "#define BMP_ID                %ld\n",  offsetof(BITMAP, bitmap_id));
   fprintf(f, "#define BMP_EXTRA             %ld\n",  offsetof(BITMAP, extra));
   fprintf(f, "#define BMP_LINEOFS           %ld\n",  offsetof(BITMAP, line_ofs));
   fprintf(f, "#define BMP_SEG               %ld\n",  offsetof(BITMAP, seg));
   fprintf(f, "#define BMP_LINE              %ld\n",  offsetof(BITMAP, line));
   fprintf(f, "\n");

   fprintf(f, "#define VTABLE_MASK_COLOR     %ld\n",  offsetof(GFX_VTABLE, mask_color));
   fprintf(f, "\n");

   fprintf(f, "#define RLE_W                 %ld\n",  offsetof(RLE_SPRITE, w));
   fprintf(f, "#define RLE_H                 %ld\n",  offsetof(RLE_SPRITE, h));
   fprintf(f, "#define RLE_DAT               %ld\n",  offsetof(RLE_SPRITE, dat));
   fprintf(f, "\n");

   fprintf(f, "#define CMP_PLANAR            %ld\n",  offsetof(COMPILED_SPRITE, planar));
   fprintf(f, "#define CMP_COLOR_DEPTH       %ld\n",  offsetof(COMPILED_SPRITE, color_depth));
   fprintf(f, "#define CMP_DRAW              %ld\n",  offsetof(COMPILED_SPRITE, proc));
   fprintf(f, "\n");

   fprintf(f, "#define IRQ_SIZE              %ld\n",  sizeof(_IRQ_HANDLER));
   fprintf(f, "#define IRQ_HANDLER           %ld\n",  offsetof(_IRQ_HANDLER, handler));
   fprintf(f, "#define IRQ_NUMBER            %ld\n",  offsetof(_IRQ_HANDLER, number));
   fprintf(f, "#define IRQ_OLDVEC            %ld\n",  offsetof(_IRQ_HANDLER, old_vector));
   fprintf(f, "\n");

   #ifdef DJGPP
      fprintf(f, "#define DPMI_AX               %ld\n",  offsetof(__dpmi_regs, x.ax));
      fprintf(f, "#define DPMI_BX               %ld\n",  offsetof(__dpmi_regs, x.bx));
      fprintf(f, "#define DPMI_CX               %ld\n",  offsetof(__dpmi_regs, x.cx));
      fprintf(f, "#define DPMI_DX               %ld\n",  offsetof(__dpmi_regs, x.dx));
      fprintf(f, "#define DPMI_SP               %ld\n",  offsetof(__dpmi_regs, x.sp));
      fprintf(f, "#define DPMI_SS               %ld\n",  offsetof(__dpmi_regs, x.ss));
      fprintf(f, "#define DPMI_FLAGS            %ld\n",  offsetof(__dpmi_regs, x.flags));
      fprintf(f, "\n");
   #endif

   fprintf(f, "#define DRAW_SOLID            %d\n",   DRAW_MODE_SOLID);
   fprintf(f, "#define DRAW_XOR              %d\n",   DRAW_MODE_XOR);
   fprintf(f, "#define DRAW_COPY_PATTERN     %d\n",   DRAW_MODE_COPY_PATTERN);
   fprintf(f, "#define DRAW_SOLID_PATTERN    %d\n",   DRAW_MODE_SOLID_PATTERN);
   fprintf(f, "#define DRAW_MASKED_PATTERN   %d\n",   DRAW_MODE_MASKED_PATTERN);
   fprintf(f, "#define DRAW_TRANS            %d\n",   DRAW_MODE_TRANS);
   fprintf(f, "\n");

   fprintf(f, "#ifndef MASK_COLOR_8\n");
   fprintf(f, "#define MASK_COLOR_8          %d\n",   MASK_COLOR_8);
   fprintf(f, "#define MASK_COLOR_15         %d\n",   MASK_COLOR_15);
   fprintf(f, "#define MASK_COLOR_16         %d\n",   MASK_COLOR_16);
   fprintf(f, "#define MASK_COLOR_24         %d\n",   MASK_COLOR_24);
   fprintf(f, "#define MASK_COLOR_32         %d\n",   MASK_COLOR_32);
   fprintf(f, "#endif\n");
   fprintf(f, "\n");

   fprintf(f, "#define POLYSEG_U             %ld\n",  offsetof(POLYGON_SEGMENT, u));
   fprintf(f, "#define POLYSEG_V             %ld\n",  offsetof(POLYGON_SEGMENT, v));
   fprintf(f, "#define POLYSEG_DU            %ld\n",  offsetof(POLYGON_SEGMENT, du));
   fprintf(f, "#define POLYSEG_DV            %ld\n",  offsetof(POLYGON_SEGMENT, dv));
   fprintf(f, "#define POLYSEG_C             %ld\n",  offsetof(POLYGON_SEGMENT, c));
   fprintf(f, "#define POLYSEG_DC            %ld\n",  offsetof(POLYGON_SEGMENT, dc));
   fprintf(f, "#define POLYSEG_R             %ld\n",  offsetof(POLYGON_SEGMENT, r));
   fprintf(f, "#define POLYSEG_G             %ld\n",  offsetof(POLYGON_SEGMENT, g));
   fprintf(f, "#define POLYSEG_B             %ld\n",  offsetof(POLYGON_SEGMENT, b));
   fprintf(f, "#define POLYSEG_DR            %ld\n",  offsetof(POLYGON_SEGMENT, dr));
   fprintf(f, "#define POLYSEG_DG            %ld\n",  offsetof(POLYGON_SEGMENT, dg));
   fprintf(f, "#define POLYSEG_DB            %ld\n",  offsetof(POLYGON_SEGMENT, db));
   fprintf(f, "#define POLYSEG_Z             %ld\n",  offsetof(POLYGON_SEGMENT, z));
   fprintf(f, "#define POLYSEG_DZ            %ld\n",  offsetof(POLYGON_SEGMENT, dz));
   fprintf(f, "#define POLYSEG_FU            %ld\n",  offsetof(POLYGON_SEGMENT, fu));
   fprintf(f, "#define POLYSEG_FV            %ld\n",  offsetof(POLYGON_SEGMENT, fv));
   fprintf(f, "#define POLYSEG_DFU           %ld\n",  offsetof(POLYGON_SEGMENT, dfu));
   fprintf(f, "#define POLYSEG_DFV           %ld\n",  offsetof(POLYGON_SEGMENT, dfv));
   fprintf(f, "#define POLYSEG_TEXTURE       %ld\n",  offsetof(POLYGON_SEGMENT, texture));
   fprintf(f, "#define POLYSEG_UMASK         %ld\n",  offsetof(POLYGON_SEGMENT, umask));
   fprintf(f, "#define POLYSEG_VMASK         %ld\n",  offsetof(POLYGON_SEGMENT, vmask));
   fprintf(f, "#define POLYSEG_VSHIFT        %ld\n",  offsetof(POLYGON_SEGMENT, vshift));
   fprintf(f, "#define POLYSEG_SEG           %ld\n",  offsetof(POLYGON_SEGMENT, seg));
   fprintf(f, "\n");

   for (x=0; x<3; x++)
      for (y=0; y<3; y++)
	 fprintf(f, "#define M_V%d%d                 %ld\n", x, y, offsetof(MATRIX_f, v[x][y]));

   for (x=0; x<3; x++)
      fprintf(f, "#define M_T%d                  %ld\n", x, offsetof(MATRIX_f, t[x]));

   fclose(f);

   return 0;
}
