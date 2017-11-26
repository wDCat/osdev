kernel_root=/home/dcat/osdev/w2
all:w2_build.jar w2_syscall.jar
w2_build.jar:src/build.kt
	kotlinc src/build.kt -include-runtime -d w2_build.jar
w2_syscall.jar:src/syscall.kt
	kotlinc src/syscall.kt -include-runtime -d w2_syscall.jar
clean:
	rm -rf $(kernel_root)/OUTPUT
	rm -rf $(kernel_root)/LOG_OUTPUT
	rm -rf ./w2_*.jar