for i in {1..16}
do
    ./build/applications/genomeseq/client client116"$i" tcp://localhost:31920 "index -s hg38.fa -p 12" &
done
wait