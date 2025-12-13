# WikiRace

A Graph based Wikipedia solver
Data from: https://dumps.wikimedia.org/enwiki/latest/  

## Reccomended specs
16gb of ram  
8 core cpu w/hyper threading  
NVME SSD  

## Setup
### Install
download the following files and run the following shell commands to setup database  
(SQLite is a required dependency)  
Note this will download ~10 gb, the database created will be 30gb  
```
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-page.sql.gz https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pagelinks.sql.gz https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-redirect.sql.gz https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-linktarget.sql.gz  
./parse.sh
./import.sh  
```

A completed database can also be found here:  
```
https://drive.google.com/file/d/1ictDdg47LH-8rf1LApjV6BU3xMDISp0z/view?usp=sharing
```

### Compile
run the provided compile script  
```
cd build
cmake ..  
cmake --build .  

```
