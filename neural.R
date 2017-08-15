library("RSQLite")
library(neuralnet)
#connecting to db
con = dbConnect(dbDriver("SQLite"), dbname = "E:/Hemanth/fra_uas/Sem_2/SSNS/20170602/Project/group-1-project/database.db")
#saving data to data frame
waterdata <-data.frame(dbGetQuery( con,'select * from log' ))
#removing the date  and serial number
waterdata <-waterdata[,-c(1:2)]
f<-(waterdata$score~ waterdata$device_id+waterdata$light + waterdata$temperature
    +waterdata$humidity+waterdata$moisture)
nn <- neuralnet(f ,data=waterdata,hidden = c(5,5),linear.output=T)
plot(nn)
