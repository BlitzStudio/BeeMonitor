package main

import (
	"context"
	"fmt"
	"log"
	"math/rand/v2"
	"time"

	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

func InsertToDb() {

	client, err := mongo.NewClient(
		options.Client().ApplyURI("mongodb://root:example@127.0.0.1:27017/"),
	)
	if err != nil {
		log.Fatal(err)
	}
	ctx := context.Background()
	err = client.Connect(ctx)
	if err != nil {
		log.Fatal(err)
	}
	defer func() {
		if err := client.Disconnect(ctx); err != nil {
			log.Fatalf("Error disconnecting from MongoDB: %v", err)
		}
	}()
	collection := client.Database("testData").Collection("temps")
	i := 0
	for i = 0; i < 100; i++ {

		beeData := new(Data)
		beeData.Name = "TestData"
		beeData.TempInside = rand.Float64() * 30
		beeData.TempOutside = 0.0
		beeData.Time = time.Now().Add(time.Duration(i) * time.Minute).Format("2006-01-02T15:04:05Z07:00")
		insertResult, err := collection.InsertOne(ctx, beeData) // Use the context
		if err != nil {
			log.Println(err) // Don't Fatal, handle the error

		}
		fmt.Println("Inserted a single document: ", insertResult.InsertedID)
	}
}
