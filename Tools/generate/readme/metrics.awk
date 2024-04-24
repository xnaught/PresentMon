BEGIN{
    FS = "\"";
    print "| Column Header          | Description |";
    print "| ---------------------- | ----------- |";
}
{
    n = split($1, a, ",")

    #printf "\n%u:%u: ", NF, n
    #for (i = 1; i <= n; ++i) printf "|%s", a[i]
    #printf("|-")
    #for (i = 2; i <= NF; ++i) printf "|%s", $i
    #printf("|\n")

    if (NF == 0) next
    if (substr(a[1], 1, 2) == "//") next

    if (NF == 1) {
        name = a[1]
        desc = a[2]
    } else if (NF == 3) {
        name = substr($1, 1, length($1)-1);
        desc = $2
    }

    if (name == "CPUStartTime") {
        name = "CPUStartTime<br>(CPUStartQPC)<br>(CPUStartQPCTime)<br>(CPUStartDateTime)"
    }

    printf "| %-22s | %s |\n", "*" name "*", desc
}
