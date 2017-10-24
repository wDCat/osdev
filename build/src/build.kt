import java.io.*
import java.util.*


/**
 * Created by DCat on 9/2/17.
 */
val ROOT = "/home/dcat/osdev/w2"
val O_OUTPUT = "${ROOT}/OUTPUT"
val LOG_OUTPUT = "${ROOT}/OUTPUT/LOG"
val Makefile = "${ROOT}/build_kernel.makefile"
val clibMakefile = "${ROOT}/build_lib.makefile"
val CMakeList = "${ROOT}/CMakeLists.txt"
var C_INCLUDE = ""
val C_FLAGS = "-fno-stack-protector -m32 -std=c99 -Wall -O0 -O -fstrength-reduce -fomit-frame-pointer -D__DCAT__ -DKERNEL=1 -finline-functions -c -nostdinc -fno-builtin"
val CLIB_C_FLAGS = "-I${ROOT}/lib/include -fPIC -fno-stack-protector -m32 -std=c99 -Wall -O -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin -nostdlib"
val NASM_FLAGS = "-f elf"
val LD_FLAGS = "n -m elf_i386 -A elf32-i386 -nostdlib"
val cFiles = Stack<String>()
val hFiles = Stack<String>()
val asmFiles = Stack<String>()
var buildStop = false
var count = 0
var fileCount = 0
fun dlog(log: Any) {
    println("[*] ${log}")
}

fun derr(log: Any) {
    colorprint("[*]", 31, 0)
    println(" ${log}")
}

fun colorprint(log: Any, fg: Int, bg: Int) {
    var bgstr = ""
    if (bg != 0)
        bgstr = "${bg};"
    print(27.toChar() + "[${bgstr}${fg}m${log}" + 27.toChar() + "[0m")
}

fun prettyOutputName(path: String): String {
    var result = path
    if (result.startsWith(ROOT))
        result = path.substring(ROOT.length + 1)
    return result.replace("[/?]".toRegex(), "_").replace("c$|asm$|s$".toRegex(), "o")
}

class ExecResult(val exitCode: Int, val output: String)

fun execWithOutput(cmd: String): ExecResult {
    //dlog(cmd)
    val proc = Runtime.getRuntime().exec(cmd)
    //用gcc编译的话会卡这...
    proc.waitFor()
    val ips = proc.inputStream
    val e = proc.errorStream
    val reader = BufferedReader(InputStreamReader(ips))
    val reader2 = BufferedReader(InputStreamReader(e))
    var line: String?
    var result = ""
    while (true) {
        line = reader.readLine()
        if (line == null) break
        result += line + "\n"
    }
    while (true) {
        line = reader2.readLine()
        if (line == null) break
        result += line + "\n"
    }
    ips.close()
    e.close()
    reader.close()
    reader2.close()
    proc.destroy()
    return ExecResult(proc.exitValue(), result)
}

class BuildThread(val id: Int) : Runnable {
    override fun run() {
        while (true) {
            if (buildStop) {
                dlog("thread[${id}] Stopped.")
                return
            }
            var path: String = ""
            synchronized(cFiles, {
                if (cFiles.isEmpty()) {
                    dlog("thread[${id}] Done.")
                    return
                }
                path = cFiles.pop()
                count++
            })
            val outputName = prettyOutputName(path)
            colorprint("[ ${((count.toFloat() / fileCount) * 100).toInt()}% ]", 37, 0)
            colorprint(" Building ${path}\n", 32, 0)
            val result = execWithOutput("gcc ${C_FLAGS} ${C_INCLUDE} -o ${O_OUTPUT}/${outputName} ${path}")
            dlog(result.output)
            if (result.exitCode != 0) {
                derr("thread[${id}] Build failed.Exited with ${result.exitCode}.")
                buildStop = true
            }

        }

    }
}

fun main(args: Array<String>) {
    val slient = true
    val dirStack = Stack<File>()
    val includeDir = Stack<String>()
    dirStack.push(File(ROOT))
    while (dirStack.isNotEmpty()) {
        dirStack.pop().listFiles().forEach {
            val fn = it.name
            if (!fn.startsWith(".") && !fn.toLowerCase().contains("cmake"))
                if ((fn.endsWith(".c") || fn.endsWith(".s")) && !fn.contains("linker_script"))
                    cFiles.add(it.absolutePath)
                else if (fn.endsWith(".i.asm"))
                    asmFiles.add(it.absolutePath)
                else if (fn.endsWith(".h"))
                    hFiles.add(it.absolutePath)
                else if (it.isDirectory) {
                    if (it.name != "build") {
                        dirStack.push(it)
                        if (it.name == "include")
                            includeDir.push(it.absolutePath)
                    }
                }

        }
    }
    dlog("found ${cFiles.count()} C source file(s) and ${asmFiles.count()} NASM source file(s).")
    includeDir.forEach {
        C_INCLUDE += "-I${it} "
    }
    fileCount = cFiles.count() + asmFiles.count()
    val cmout = FileWriter(File(CMakeList))
    cmout.write("cmake_minimum_required(VERSION 3.6)\n" +
            "project(W2)\n" +
            "set(CMAKE_C_FLAGS \"${C_FLAGS}\")\n")
    includeDir.forEach {
        cmout.write("include_directories(${it})\n")
    }
    cmout.write("FILE(GLOB MyCSources \n")
    val mfout = FileWriter(File(Makefile))
    mfout.write("build_kernel:\n")
    mfout.write("\tmkdir  ${O_OUTPUT}||echo \"\"\n")
    mfout.write("\trm -rf ${O_OUTPUT}/*\n")
    mfout.write("\tmkdir  ${LOG_OUTPUT}||echo \"\"\n")
    mfout.write("\trm -rf ${LOG_OUTPUT}/*\n")
    val clibmfout = FileWriter(File(clibMakefile))
    clibmfout.write("build_lib:\n")
    clibmfout.write("\t@echo \"[ %%% ] \\033[32m Building clib...\\033[0m\"\n")
    clibmfout.write("\t@gcc -shared -o ${ROOT}/libdcat.so  ${CLIB_C_FLAGS} ")
    cFiles.forEach {
        val prog = ((count.toFloat() / fileCount) * 100).toInt()
        count++
        val outputName = prettyOutputName(it)
        mfout.write("\t@echo \"[ $prog% ] \\033[32m Building ${it}\\033[0m\"\n")
        if (slient)
            mfout.write("\t@gcc ${C_FLAGS} ${C_INCLUDE} -o ${O_OUTPUT}/${outputName} ${it} 1>${LOG_OUTPUT}/${outputName}.log 2>&1 || echo ''\n")
        else
            mfout.write("\tgcc ${C_FLAGS} ${C_INCLUDE} -o ${O_OUTPUT}/${outputName} ${it} 2>&1 | tee ${LOG_OUTPUT}/${outputName}.log\n")
        mfout.write("\t@if [ -e \"${O_OUTPUT}/${outputName}\" ];" +
                "then " +
                "echo \"[ $prog% ] \\033[32m Build Done: ${it}\\033[0m\";" +
                "else " +
                "echo \"\\033[31m Build Output: \\033[0m\" &&" +
                " cat ${LOG_OUTPUT}/${outputName}.log &&" +
                " echo \"\\033[31m Build Failed: ${it}\\033[0m\" &&" +
                " exit 1;" +
                "fi\n")
        cmout.write("${it}\n")
        if ((it.startsWith("${ROOT}/lib/") || it.startsWith("${ROOT}lib/"))) {
            clibmfout.write(it + " ")
        }
    }
    clibmfout.write("1>${LOG_OUTPUT}/clib.log 2>&1 || echo ''\n")
    clibmfout.write("\t@if [ -e \"${ROOT}/libdcat.so\" ];" +
            "then " +
            "echo \"[ %%% ] \\033[32m Build Lib Done!\\033[0m\";" +
            "else " +
            "echo \"\\033[31m Build Output: \\033[0m\" &&" +
            " cat ${LOG_OUTPUT}/clib.log &&" +
            " echo \"\\033[31m Build Lib Failed!!\\033[0m\" &&" +
            " exit 1;" +
            "fi\n")
    hFiles.forEach {
        cmout.write("${it}\n")
    }
    asmFiles.forEach {
        count++
        val outputName = prettyOutputName(it)
        mfout.write("\tnasm ${NASM_FLAGS} -o ${O_OUTPUT}/${outputName} ${it}\n")
    }
    clibmfout.flush()
    clibmfout.close()
    mfout.flush()
    mfout.close()

    cmout.write(")\n" +
            "ADD_EXECUTABLE(MyExecutable \${MyCSources})")
    cmout.flush()
    cmout.close()
    dlog("Makefile and CMakeLists.txt updated.")

}