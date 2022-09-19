CFLAGS = -std=c99 -O2
LDFLANGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: main.c
	gcc $(CFLAGS) -o VulkanTest main.c $(LDFLANGS)

.PHONY: test clean

test: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest