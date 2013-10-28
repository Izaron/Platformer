#include "level.h"
#include <Box2D\Box2D.h>

#include <iostream>
#include <random>

Object player;
b2Body* playerBody;

std::vector<Object> coin;
std::vector<b2Body*> coinBody;

std::vector<Object> enemy;
std::vector<b2Body*> enemyBody;

int main()
{
	srand(time(NULL));

	Level lvl;
	lvl.LoadFromFile("platformer.tmx");


    b2Vec2 gravity(0.0f, 1.0f);
    b2World world(gravity);

	sf::Vector2i tileSize = lvl.GetTileSize();

	std::vector<Object> block = lvl.GetObjects("block");
	for(int i = 0; i < block.size(); i++)
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_staticBody;
		bodyDef.position.Set(block[i].rect.left + tileSize.x / 2 * (block[i].rect.width / tileSize.x - 1),
			block[i].rect.top + tileSize.y / 2 * (block[i].rect.height / tileSize.y - 1));
		b2Body* body = world.CreateBody(&bodyDef);
		b2PolygonShape shape;
		shape.SetAsBox(block[i].rect.width / 2, block[i].rect.height / 2);
		body->CreateFixture(&shape,1.0f);
	}

	coin = lvl.GetObjects("coin");
	for(int i = 0; i < coin.size(); i++)
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(coin[i].rect.left + tileSize.x / 2 * (coin[i].rect.width / tileSize.x - 1),
			coin[i].rect.top + tileSize.y / 2 * (coin[i].rect.height / tileSize.y - 1));
		bodyDef.fixedRotation = true;
		b2Body* body = world.CreateBody(&bodyDef);
		b2PolygonShape shape;
		shape.SetAsBox(coin[i].rect.width / 2, coin[i].rect.height / 2);
		body->CreateFixture(&shape,1.0f);
		coinBody.push_back(body);
	}

	enemy = lvl.GetObjects("enemy");
	for(int i = 0; i < enemy.size(); i++)
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(enemy[i].rect.left +
			tileSize.x / 2 * (enemy[i].rect.width / tileSize.x - 1),
			enemy[i].rect.top + tileSize.y / 2 * (enemy[i].rect.height / tileSize.y - 1));
		bodyDef.fixedRotation = true;
		b2Body* body = world.CreateBody(&bodyDef);
		b2PolygonShape shape;
		shape.SetAsBox(enemy[i].rect.width / 2, enemy[i].rect.height / 2);
		body->CreateFixture(&shape,1.0f);
		enemyBody.push_back(body);
	}


	player = lvl.GetObject("player");
	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(player.rect.left, player.rect.top);
	bodyDef.fixedRotation = true;
	playerBody = world.CreateBody(&bodyDef);
	b2PolygonShape shape; shape.SetAsBox(player.rect.width / 2, player.rect.height / 2);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = 1.0f; fixtureDef.friction = 0.3f;
	playerBody->CreateFixture(&fixtureDef);



	sf::Vector2i screenSize(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenSize.x, screenSize.y), "Game");


	sf::View view;
	view.reset(sf::FloatRect(0.0f, 0.0f, screenSize.x, screenSize.y));
	view.setViewport(sf::FloatRect(0.0f, 0.0f, 2.0f, 2.0f));

    while(window.isOpen())
    {
        sf::Event evt;

        while(window.pollEvent(evt))
        {
			switch(evt.type)
			{
			case sf::Event::Closed:
                window.close();
				break;

			case sf::Event::KeyPressed:
				if(evt.key.code == sf::Keyboard::W && playerBody->GetLinearVelocity().y == 0)
					playerBody->SetLinearVelocity(b2Vec2(0.0f, -15.0f));

				if(evt.key.code == sf::Keyboard::D)
					playerBody->SetLinearVelocity(b2Vec2(5.0f, 0.0f));

				if(evt.key.code == sf::Keyboard::A)
					playerBody->SetLinearVelocity(b2Vec2(-5.0f, 0.0f));
				break;
			}
        }

		world.Step(1.0f / 60.0f, 1, 1);


		for(b2ContactEdge* ce = playerBody->GetContactList(); ce; ce = ce->next)
		{
			b2Contact* c = ce->contact;
			
			for(int i = 0; i < coinBody.size(); i++)
				if(c->GetFixtureA() == coinBody[i]->GetFixtureList())
				{
					coinBody[i]->DestroyFixture(coinBody[i]->GetFixtureList());
					coin.erase(coin.begin() + i);
					coinBody.erase(coinBody.begin() + i);
				}

			for(int i = 0; i < enemyBody.size(); i++)
				if(c->GetFixtureA() == enemyBody[i]->GetFixtureList())
				{
					if(playerBody->GetPosition().y < enemyBody[i]->GetPosition().y)
					{
						playerBody->SetLinearVelocity(b2Vec2(0.0f, -10.0f));

						enemyBody[i]->DestroyFixture(enemyBody[i]->GetFixtureList());
						enemy.erase(enemy.begin() + i);
						enemyBody.erase(enemyBody.begin() + i);
					}
					else
					{
						int tmp = (playerBody->GetPosition().x < enemyBody[i]->GetPosition().x)
							? -1 : 1;
						playerBody->SetLinearVelocity(b2Vec2(10.0f * tmp, 0.0f));
					}
				}
		}

		for(int i = 0; i < enemyBody.size(); i++)
		{
			if(enemyBody[i]->GetLinearVelocity() == b2Vec2_zero)
			{
				int tmp = (rand() % 2 == 1) ? 1 : -1;
				enemyBody[i]->SetLinearVelocity(b2Vec2(5.0f * tmp, 0.0f));
			}
		}


		b2Vec2 pos = playerBody->GetPosition();
		view.setCenter(pos.x + screenSize.x / 4, pos.y + screenSize.y / 4);
		window.setView(view);

		player.sprite.setPosition(pos.x, pos.y);

		for(int i = 0; i < coin.size(); i++)
			coin[i].sprite.setPosition(coinBody[i]->GetPosition().x, coinBody[i]->GetPosition().y);

		for(int i = 0; i < enemy.size(); i++)
			enemy[i].sprite.setPosition(enemyBody[i]->GetPosition().x, enemyBody[i]->GetPosition().y);

        window.clear();

		lvl.Draw(window);

		window.draw(player.sprite);

		for(int i = 0; i < coin.size(); i++)
			window.draw(coin[i].sprite);

		for(int i = 0; i < enemy.size(); i++)
			window.draw(enemy[i].sprite);

		window.display();
    }

    return 0;
}