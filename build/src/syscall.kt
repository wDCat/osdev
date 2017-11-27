import java.io.*
import java.text.DateFormat
import java.text.SimpleDateFormat
import java.util.*
import kotlin.system.exitProcess

/**
 * Created by DCat on 11/24/17.
 */
class SyscallNos {
    val map = LinkedHashMap<String, String>()
    fun add(name: String, no: String) = map.put(name, no)
    fun remove(name: String) = map.remove(name)
    fun addHeaderFile(path: String) {
        var count = 0
        val fin = BufferedReader(FileReader(File(path)))
        while (true) {
            val line = (fin.readLine() ?: break).trim()
            //println(line)
            if (line.length < 1 || !line.startsWith("#define")) continue
            val name: String
            var no: String
            var i = line.indexOf(' ')
            if (i < 0) continue
            val lesC = line.substring(i + 1)
            i = lesC.indexOf(' ')
            if (i < 0) {
                i = lesC.indexOf('\t')
                if (i < 0)
                    continue
            }
            val sname = lesC.substring(0, i).trim()
            no = lesC.substring(i + 1).trim()
            if (sname.trim().startsWith("__NR_")) {
                name = sname.substring(5)
            } else if (sname.startsWith("SYS_")) {
                name = sname.substring(4)
            } else name = sname
            //println("syscall ${name} ${no}")
            if (map.containsKey(name)) map.remove(name)
            add(name, no)
            count++
        }
        println("[*] add ${path} done. syscall count:${count}")
    }

    fun writeToHeaderFile(path: String) {
        val file = File(path)
        if (!file.exists()) file.createNewFile()
        val out = PrintStream(FileOutputStream(file))
        out.println("// Syscall No Defines")
        out.println("// Generated By DCat's Little Script ${SimpleDateFormat("yyyy/MM/dd HH:mm").format(Date())}")
        out.println()
        out.println()
        map.forEach { name, no ->
            out.println("#define SYS_${name} ${no}")
        }
        out.println()
        map.forEach { name, no ->
            out.println("#define __NR_${name} ${no}")
        }
        out.println()
    }

    fun exportNameTable(path: String) {
        val t = TreeMap<Int, String>(kotlin.Comparator { o1, o2 -> o1 - o2 })
        map.forEach { name, no ->
            try {
                print("[${no}]")
                val intno = no.trim().toInt()
                println("[${intno}]$name")
                t[intno] = name
            } catch (ignored: Throwable) {

            }
        }
        val file = File(path)
        if (!file.exists()) file.createNewFile()
        val out = PrintStream(FileOutputStream(file))
        out.println("// Syscall No Defines Table")
        out.println("// Generated By DCat's Little Script ${SimpleDateFormat("yyyy/MM/dd HH:mm").format(Date())}")
        out.println()
        out.println()
        out.println("const char* SYSCALL_NAMES_TABLE[]={")
        for (x in 0..t.size) {
            if (!t.containsKey(x)) {
                out.println("\"UNDEF\",")
            } else {
                out.println("\"${t[x]}\",")
            }
        }
        out.println("};")
        out.println("#define SYSCALL_NAMES_COUNT (sizeof(SYSCALL_NAMES_TABLE)/sizeof(const char*))")
    }


}

fun main(args: Array<String>) {

    var output = ""
    var output2 = ""
    val inputs = Stack<String>()
    var x = 0
    while (x < args.size) {
        if (args[x].equals("--output")) {
            output = args[x + 1]
            x += 2
            continue
        }
        if (args[x].equals("--table")) {
            output2 = args[x + 1]
            x += 2
            continue
        }
        inputs.push(args[x])
        x++
    }
    if ((output.length < 1 && output2.length < 1) || inputs.size < 1) {
        println("Bad args...\n")
        exitProcess(1)
    }
    val st = SyscallNos()
    inputs.forEach {
        println("[*] processing ${it}")
        st.addHeaderFile(it)
    }
    if (output.length > 0) {
        st.writeToHeaderFile(output)
        println("[*] writed to ${output}")
    }
    if (output2.length > 0) {
        st.exportNameTable(output2)
        println("[*] writed to ${output2}")
    }



    exitProcess(0)
    /*
    st.addHeaderFile("/home/dcat/osdev/musl/arch/i386/bits/syscall.linux.h.in")
    st.addHeaderFile("/home/dcat/osdev/w2/lib/include/syscallno.h")
    st.writeToHeaderFile("/home/dcat/osdev/musl/obj/include/bits/syscall.h")*/
}