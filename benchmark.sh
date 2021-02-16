for i in {1..2}
do
    ./build/applications/genomeseq/client client"$i" tcp://localhost:31920 "index -p hg38.fa -p 12" &
done
wait