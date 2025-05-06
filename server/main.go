package main

import (
	"context"
	"fmt"
	"log"

	"github.com/davecgh/go-spew/spew"
	"github.com/gofiber/fiber/v2"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

type Data struct {
	Name        string  `bson:"name" json:"name"`
	TempInside  float64 `bson:"tempInside" json:"tempInside"`
	TempOutside float64 `bson:"tempOutside" json:"tempOutside"`
	Time        string  `bson:"date" json:"time"`
}

func main() {
	// MongoDB connection
	client, err := mongo.NewClient(
		options.Client().ApplyURI("mongodb://root:example@mongo:27017/"),
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

	// Fiber instance
	app := fiber.New()

	// Routes
	app.Get("/", func(c *fiber.Ctx) error {
		testData := Data{Name: "main", TempInside: 33.5, TempOutside: 23.5}

		insertResult, err := collection.InsertOne(ctx, testData) // Use the context
		if err != nil {
			log.Println(err) // Don't Fatal, handle the error
			return c.Status(fiber.StatusInternalServerError).SendString("Failed to insert data")
		}
		fmt.Println("Inserted a single document: ", insertResult.InsertedID)

		//Find the document you just inserted
		var result Data
		filter := bson.M{"_id": insertResult.InsertedID} // Filter by inserted ID
		err = collection.FindOne(ctx, filter).Decode(&result)

		if err != nil {
			log.Println(err) // Handle the error, don't Fatal
			return c.Status(fiber.StatusInternalServerError).SendString("Failed to find data")
		}

		fmt.Printf("Found a single document: %+v\n", result)

		return c.SendString("Hello, MongoDB!")
	})

	app.Post("/data", func(c *fiber.Ctx) error {
		spew.Dump(c.Body())
		fmt.Println(string(c.Body()))
		beeData := new(Data)
		if err := c.BodyParser(beeData); err != nil {
			fmt.Println("Error: ", err)
			return c.SendStatus(200)
		}
		spew.Dump(beeData)
		insertResult, err := collection.InsertOne(ctx, beeData) // Use the context
		if err != nil {
			log.Println(err) // Don't Fatal, handle the error
			return c.Status(fiber.StatusInternalServerError).SendString("Failed to insert data")
		}
		fmt.Println("Inserted a single document: ", insertResult.InsertedID)

		return c.SendStatus(200)
	})

	// Start server
	log.Fatal(app.Listen(":8030"))
}
