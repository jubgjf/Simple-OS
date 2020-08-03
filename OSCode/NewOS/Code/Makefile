TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

#默认动作

default :
	$(MAKE) haribote.img

#文件生成规则

haribote.img : haribote/ipl.bin haribote/haribote.sys Makefile \
		help/help.hrb time/time.hrb type/type.hrb cal/cal.hrb \
		demo/hello.c demo/ubuntu.txt demo/debian.txt demo/ubian.txt demo/logo.txt
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:haribote/ipl.bin len:512 from:0 to:0 \
		copy from:haribote/haribote.sys to:@: \
		copy from:help/help.hrb to:@: \
		copy from:time/time.hrb to:@: \
		copy from:type/type.hrb to:@: \
		copy from:cal/cal.hrb to:@: \
		copy from:demo/hello.c to:@: \
		copy from:demo/ubuntu.txt to:@: \
		copy from:demo/debian.txt to:@: \
		copy from:demo/ubian.txt to:@: \
		copy from:demo/logo.txt to:@: \
		imgout:haribote.img

#命令

run :
	$(MAKE) haribote.img
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) haribote.img

full :
	$(MAKE) -C haribote
	$(MAKE) -C apilib
	$(MAKE) -C help
	$(MAKE) -C time
	$(MAKE) -C type
	$(MAKE) -C cal
	$(MAKE) haribote.img

run_full :
	$(MAKE) full
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install_full :
	$(MAKE) full

run_os :
	$(MAKE) -C haribote
	$(MAKE) run

clean :
#不执行任何操作

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img

clean_full :
	$(MAKE) -C haribote		clean
	$(MAKE) -C apilib		clean
	$(MAKE) -C help			clean
	$(MAKE) -C time			clean
	$(MAKE) -C type			clean
	$(MAKE) -C cal			clean

src_only_full :
	$(MAKE) -C haribote		src_only
	$(MAKE) -C apilib		src_only
	$(MAKE) -C help			src_only
	$(MAKE) -C time			src_only
	$(MAKE) -C type			src_only
	$(MAKE) -C cal			src_only
	-$(DEL) haribote.img

refresh :
	$(MAKE) full
	$(MAKE) clean_full
	-$(DEL) haribote.img
