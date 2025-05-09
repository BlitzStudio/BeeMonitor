package main

import (
	"context"
	"fmt"
	"log"
	"strings"
	"time"

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
	// InsertToDb()
	// MongoDB connection
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

	// Fiber instance
	app := fiber.New()

	// Routes
	app.Get("/", func(c *fiber.Ctx) error {
		// Calculate the time 30 minutes ago
		thirtyMinutesAgo := time.Now().Add(-30 * time.Minute).Format(
			time.RFC3339,
		)

		// Construct the filter to get data within the last 30 minutes
		filter := bson.M{"date": bson.M{"$gte": thirtyMinutesAgo}}

		// Fetch data from MongoDB based on the filter
		cursor, err := collection.Find(ctx, filter)
		if err != nil {
			log.Println(err)
			return c.Status(
				fiber.StatusInternalServerError,
			).SendString("Failed to retrieve data")
		}
		defer cursor.Close(ctx)

		var results []Data
		if err := cursor.All(ctx, &results); err != nil {
			log.Println(err)
			return c.Status(
				fiber.StatusInternalServerError,
			).SendString("Failed to decode data")
		}
		spew.Dump(results)
		// Return the data as JSON
		return c.JSON(results)
	})

	app.Post("/", func(c *fiber.Ctx) error {
		beeData := new(Data)
		if err := c.BodyParser(beeData); err != nil {
			fmt.Println("Error: ", err)
			return c.SendStatus(200)
		}
		date := beeData.Time
		date = strings.ReplaceAll(date, "/", "-")
		date = strings.ReplaceAll(date, ",", "T")
		beeData.Time = date[:len(date)-3]
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
