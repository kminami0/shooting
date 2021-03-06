﻿#include <Siv3D.hpp> // OpenSiv3D v0.4.3

struct GameData
{
    int32 score = 0;
    int32 highscore = 0;
};

using App = SceneManager<String, GameData>;

bool gameover = false; //ゲームオーバー判定
bool gameclear = false; //ゲームクリア判定

// タイトルシーン
class Title : public App::Scene
{
public:

    // コンストラクタ（必ず実装）
    Title(const InitData& init)
        : IScene(init)
    {
        gameover = false;
        gameclear = false;
        ClearPrint();
        Print << U"high score: " << getData().highscore; //ハイスコア表示
        AudioAsset(U"title_BGM").setVolume(0.3);
        AudioAsset(U"title_BGM").play();
    }

    // 更新関数
    void update() override
    {
        // 左クリックで
        if (MouseL.down())
        {
            AudioAsset(U"title_BGM").stop();
            getData().score = 0;
            // ゲームシーンに遷移
            changeScene(U"Game");
        }


        // Sキーで
        if (KeyS.down())
        {
            AudioAsset(U"title_BGM").stop();
            // ランキングシーンに遷移
            changeScene(U"Ranking");
        }
    }

    // 描画関数 (const 修飾)
    void draw() const override
    {
        Scene::SetBackground(ColorF(0.3, 0.4, 0.5));

        FontAsset(U"BigFont")(U"Shooting Game").drawAt(400, 100);
        FontAsset(U"BigFont")(U"Click to start").drawAt(400, 300);
        FontAsset(U"BigFont")(U"Press S to see the ranking").drawAt(400, 500);
    }
};

//ランキングシーン
class Ranking : public App::Scene
{
public:

    // コンストラクタ（必ず実装）
    Ranking(const InitData& init)
        : IScene(init)
    {
        ClearPrint();
        
        // ランキングファイルをオープンする
        TextReader reader(U"ranking.txt");

        // オープンに失敗
        if (!reader)
        {
            throw Error(U"Failed to open `ranking.txt`");
        }

        // 行の内容を読み込む変数
        String line;

        Array<String> names;
        Array<int32> scores;

        Array<std::pair<int32, String>> ranking;
        int32 cnt = 0;

        // 終端に達するまで 1 行ずつ読み込む
        while (reader.readLine(line))
        {
            if (cnt % 2 == 0) {
                scores << Parse<int32>(line);
            }
            else {
                names << line;
            }
            cnt++;
        }

        for (auto i : step(cnt/2)) {
            ranking << std::make_pair(scores[i], names[i]);
        }

        sort(ranking.begin(), ranking.end());
        reverse(ranking.begin(), ranking.end());

        for (auto i : step(cnt / 2)) {
            Print << i+1 << U": " << ranking[i].second << U" " << ranking[i].first;
        }

        Print << U"Press T to back to title";

        AudioAsset(U"ranking_BGM").setVolume(0.3);
        AudioAsset(U"ranking_BGM").play();
    }

    // 更新関数
    void update() override
    {
        // Tキーで
        if (KeyT.down())
        {
            AudioAsset(U"ranking_BGM").stop();
            // タイトルシーンに遷移
            changeScene(U"Title");
        }
    }

    // 描画関数 (const 修飾)
    void draw() const override
    {
        Scene::SetBackground(ColorF(0.3, 0.4, 0.5));

    }
};

//スコア登録シーン
class ScoreRegister : public App::Scene
{
public:
    
    String name;
    bool regi = false;

    // コンストラクタ（必ず実装）
    ScoreRegister(const InitData& init)
        : IScene(init)
    {
        ClearPrint();

    }

    // 更新関数
    void update() override
    {
        // 右クリックで
        if (MouseR.down())
        {
            AudioAsset(U"ranking_BGM").stop();
            // タイトルシーンに遷移
            changeScene(U"Title");
        }

        // キーボードから名前を入力
        TextInput::UpdateText(name);

        if (SimpleGUI::Button(U"Register", Vec2(100, 100)) && !regi)
        {
            // 追加モードでテキストファイルをオープン
            TextWriter writer(U"ranking.txt", OpenMode::Append);
            // オープンに失敗
            if (!writer)
            {
                throw Error(U"Failed to open `ranking.txt`");
            }
            // 文章を追加する
            writer << getData().score;
            writer << name;
            regi = true;
        }
        
    }

    // 描画関数 (const 修飾)
    void draw() const override
    {
        Scene::SetBackground(ColorF(0.3, 0.4, 0.5));
        
        ClearPrint();

        Print << U"Enter your name.";

        Print << name;

        if (regi) {
            Print << U"You have successfully registered!";
        }

        Print << U"Right click to back to title";
    }
};

//自機
class Player {
public:
    Vec2 pos;
    const double speed = 5; //自機のスピード
    Circle circle;
    int32 life;
    bool invincible;

    Player() :
        pos(320, 240),
        circle(pos, 30),
        life(3),
        invincible(false)
    {

    }
    void update() {
        //自機の移動
        if (KeyUp.pressed()) {
            pos.y -= speed;
        }
        if (KeyDown.pressed()) {
            pos.y += speed;
        }
        if (KeyLeft.pressed()) {
            pos.x -= speed;
        }
        if (KeyRight.pressed()) {
            pos.x += speed;
        }
        //自機が画面外に出るとゲームオーバー
        if (pos.x > Scene::Width() || pos.x < 0 || pos.y > Scene::Height() || pos.y < 0) {
            gameover = true;
        }
        circle = Circle(pos, 30.0);
    }
    //自機（円）を描画
    void draw() const{
        if (invincible) {
            //0.1秒点灯、0.1秒消灯で点滅
            if (Periodic::Square0_1(0.2s)) {
                circle.draw(Color(0, 0, 255));
            }
        }
        else {
            circle.draw(Color(0, 0, 255));
        }
    }
};

Stopwatch stopwatch;

//アイテム
class Item {
public:
    Vec2 pos;
    Vec2 velocity; //アイテムの速度
    int32 kind;
    Circle circle;

    Item(Vec2 _pos, int32 _kind) :
        pos(_pos),
        velocity(Vec2(0, 0)),
        kind(_kind),
        circle(pos, 10)
    {
    }

    void update() {
        if (kind == 1) {
            velocity = Vec2(sin(stopwatch.sF()), cos(stopwatch.sF()));
        }
        pos += velocity;
        circle = Circle(pos, 30);
    }

    void draw() const {
        if (kind == 1) {
            TextureAsset(U"heart").scaled(0.5).drawAt(pos);
        }
    }
};

//敵
class Enemy {
public:
    Vec2 pos;
    Vec2 velocity; //敵の速度
    Player* pPlayer;
    Circle circle;
    int32 kind;
    int32 hp;
    Polygon cross;
    Polygon plus;

    Enemy(Vec2 _pos, int32 _kind, int32 _hp) :
        pos(_pos),
        velocity(Vec2(0, 0)),
        circle(pos, 30),
        kind(_kind),
        hp(_hp)
    {
    }

    void update() {
        //敵の移動
        if (kind == 1) {
            double dx = pPlayer->pos.x - pos.x;
            double dy = pPlayer->pos.y - pos.y;
            velocity = Vec2(dx / pow(dx * dx + dy * dy, 0.5), dy / pow(dx * dx + dy * dy, 0.5));
        }
        if (kind == 2) {
            double dx = 1;
            double dy = cos(stopwatch.sF());
            velocity = Vec2(dx / pow(dx * dx + dy * dy, 0.5), dy / pow(dx * dx + dy * dy, 0.5));
        }
        if (kind == 3) {
            velocity = Vec2(sin(stopwatch.sF()), cos(stopwatch.sF()));
        }
        if (kind == 4) {
            velocity = RandomVec2(2);
        }
        if (kind == 5) {
            double dx = pPlayer->pos.x - pos.x;
            double dy = pPlayer->pos.y - pos.y;
            velocity = Vec2(dx / pow(dx * dx + dy * dy, 0.5), dy / pow(dx * dx + dy * dy, 0.5)) + Vec2(sin(stopwatch.sF()), cos(stopwatch.sF()));
        }
        pos += velocity;
        circle = Circle(pos, 30.0);
        cross = Shape2D::Cross(80, 10, pos);
        plus = Shape2D::Plus(80, 10, pos);
    }
    //敵を描画
    void draw() const{
        if (kind == 1) {
            circle.draw(Color(255, 0, 0));
        }
        if (kind == 2) {
            circle.draw(Color(0, 255, 0));
        }
        if (kind == 3) {
            circle.draw(Color(255, 255, 0));
        }
        if (kind == 4) {
            cross.draw(Palette::Skyblue);
        }
        if (kind == 5) {
            plus.draw(Palette::Teal);
        }
    }

    //ポインタ取得用関数
    void setPlayerPtr(Player* ptr) {
        pPlayer = ptr;
    }
};

//敵ショット
class EnemyBullet {
public:
    Vec2 pos;
    Vec2 vel; //敵ショットの速度
    Circle circle;

    EnemyBullet(Vec2 _pos, Vec2 _vel) :
        pos(_pos),
        vel(_vel)
    {
    }

    void update() {
        //敵ショットの移動
        pos += vel;
        circle = Circle(pos, 3.0);
    }

    //敵ショットの描画
    void draw() const{
        circle.draw(Color(0, 0, 0));
    }
};

class ItemManager {
public:
    Array<Item> items;

    void update() {
        for (auto i : step(items.size())) {
            items[i].update();
        }
        //画面外のアイテムは消滅
        items.remove_if([](Item e) {return e.pos.x > Scene::Width(); });
        items.remove_if([](Item e) {return e.pos.x < 0; });
        items.remove_if([](Item e) {return e.pos.y > Scene::Height(); });
        items.remove_if([](Item e) {return e.pos.y < 0; });
    }

    void draw() const {
        for (auto i : step(items.size())) {
            items[i].draw();
        }
        
    }

    void add(Item item) {
        items << item;
    }

};

double enemyShotTime = 0.5; //敵ショットの間隔
double enemyShotTimer = 0; //敵ショットの間隔タイマー
double invincibleTime = 2.0; //無敵時間
double invincibleTimer = 0; //無敵時間タイマー

class EnemyManager {
public:
    Array<Enemy> enemies;
    Array<EnemyBullet> enemybullets;

    void update() {
        for (auto i : step(enemies.size())) {
            enemies[i].update();
        }
        //画面外の敵は消滅
        enemies.remove_if([](Enemy e) {return e.pos.x > Scene::Width(); });
        enemies.remove_if([](Enemy e) {return e.pos.x < 0; });
        enemies.remove_if([](Enemy e) {return e.pos.y > Scene::Height(); });
        enemies.remove_if([](Enemy e) {return e.pos.y < 0; });

        //敵ショットの発射
        if (enemyShotTimer > enemyShotTime) {
            for (auto i : step(enemies.size())) {
                if (enemies[i].kind <= 3) {
                    Vec2 vel = RandomVec2(5);
                    EnemyBullet enemybullet(enemies[i].pos + 6 * vel, vel);
                    enemybullets << enemybullet;
                }
                else if (enemies[i].kind == 4) {
                    for (auto j : step(4)) {
                        Vec2 vel(5 * cos(j * 90_deg + 45_deg), 5 * sin(j * 90_deg + 45_deg));
                        EnemyBullet enemybullet(enemies[i].pos + 16 * vel, vel);
                        enemybullets << enemybullet;
                    }
                    Vec2 vel = RandomVec2(5);
                    EnemyBullet enemybullet(enemies[i].pos, vel);
                    enemybullets << enemybullet;
                }
                else if (enemies[i].kind == 5) {
                    for (auto j : step(4)) {
                        Vec2 vel(5 * cos(j * 90_deg), 5 * sin(j * 90_deg));
                        EnemyBullet enemybullet(enemies[i].pos + 16 * vel, vel);
                        enemybullets << enemybullet;
                    }
                    Vec2 vel = RandomVec2(5);
                    EnemyBullet enemybullet(enemies[i].pos, vel);
                    enemybullets << enemybullet;
                }
            }
            enemyShotTimer -= enemyShotTime;
        }
        for (auto i : step(enemybullets.size())) {
            enemybullets[i].update();
        }
    }

    void draw() const{
        for (auto i : step(enemies.size())) {
            enemies[i].draw();
        }
        for (auto i : step(enemybullets.size())) {
            enemybullets[i].draw();
        }
    }

    void add(Enemy enemy) {
        enemies << enemy;
    }

    int32 boss_hp() const{
        for (auto i : step(enemies.size())) {
            if (enemies[i].kind >= 4) {
                return enemies[i].hp;
            }
        }
        return -1;
    }
};

//自機ショット
class Bullet {
public:
    Vec2 pos;
    double speed; //自機ショットのスピード
    Circle circle;
    int32 type;

    Bullet(Vec2 _pos, int32 _type) :
        pos(_pos),
        speed(5),
        circle(pos, 3),
        type(_type)
    {
    }

    void update() {
        //自機ショットの移動
        if (type == 0) {
            pos += Vec2(0, -speed);
        }
        if (type == 1) {
            pos += Vec2(speed, 0);
        }
        if (type == 2) {
            pos += Vec2(0, speed);
        }
        if (type == 3) {
            pos += Vec2(-speed, 0);
        }
        circle = Circle(pos, 3.0);
    }

    //自機ショットの描画
    void draw() const{
        circle.draw(Color(255, 255, 255));
    }
};

class BulletManager {
public:
    Array<Bullet> bullets;

    void update() {
        for (auto i : step(bullets.size())) {
            bullets[i].update();
        }
        //画面外の自機ショットは消滅
        bullets.remove_if([](Bullet b) {return b.pos.x > Scene::Width(); });
        bullets.remove_if([](Bullet b) {return b.pos.x < 0; });
        bullets.remove_if([](Bullet b) {return b.pos.y > Scene::Height(); });
        bullets.remove_if([](Bullet b) {return b.pos.y < 0; });
    }

    void draw() const{
        for (auto i : step(bullets.size())) {
            bullets[i].draw();
        }
    }

    void add(Bullet bullet) {
        bullets << bullet;
    }
};

double enemySpawnTime = 2; //敵の発生間隔

struct Particle
{
    Vec2 start;

    Vec2 velocity;
};

//https://siv3d.github.io/ja-jp/tutorial/effect/#164
struct Spark : IEffect
{
    Array<Particle> m_particles;

    Spark(const Vec2& start)
        : m_particles(50)
    {
        for (auto& particle : m_particles)
        {
            particle.start = start + RandomVec2(10.0);

            particle.velocity = RandomVec2(1.0) * Random(80.0);
        }
    }

    bool update(double t) override
    {
        for (const auto& particle : m_particles)
        {
            const Vec2 pos = particle.start
                + particle.velocity * t + 0.5 * t * t * Vec2(0, 240);

            Triangle(pos, 16.0, pos.x * 5_deg).draw(HSV(pos.y - 40).toColorF(1.0 - t));
        }

        return t < 1.0;
    }
};

//アイテム獲得判定
void GetDetection(ItemManager* itemManager, Player* player) {
    if (itemManager->items.isEmpty()) {
        return;
    }
    
    for (auto it = itemManager->items.begin(); it != itemManager->items.end();) {
        if ((it->circle).intersects(player->circle)) {
            player->life++;
            itemManager->items.erase(it);
            break;
        }
        else {
            it++;
        }
    }
    return;
}

//衝突判定
void CollisionDetection(EnemyManager* enemyManager, BulletManager* bulletManager, Player* player, Effect* effect, int32* score, int32* highscore) {
    if (enemyManager->enemies.isEmpty()) {
        return;
    }
    //敵と自機の衝突判定
    for (auto it = enemyManager->enemies.begin(); it != enemyManager->enemies.end();) {
        if (player->invincible) {
            break;
        }
        if ((it->circle).intersects(player->circle) && it->kind <= 3) {
            player->life--;
            //エフェクト
            effect->add<Spark>(player->pos);
            AudioAsset(U"explosion2").playOneShot();
            player->invincible = true;
            if (player->life <= 0) {
                gameover = true;
            }
            break;
        }
        else if ((it->cross).intersects(player->circle) && it->kind == 4) {
            player->life--;
            //エフェクト
            effect->add<Spark>(player->pos);
            AudioAsset(U"explosion2").playOneShot();
            player->invincible = true;
            if (player->life <= 0) {
                gameover = true;
            }
            break;
        }
        else if ((it->plus).intersects(player->circle) && it->kind == 5) {
            player->life--;
            //エフェクト
            effect->add<Spark>(player->pos);
            AudioAsset(U"explosion2").playOneShot();
            player->invincible = true;
            if (player->life <= 0) {
                gameover = true;
            }
            break;
        }
        else {
            it++;
        }
    }

    //敵と自機ショットの衝突判定
    if (!enemyManager->enemies.isEmpty() && !bulletManager->bullets.isEmpty()) {
        bool hit = false;
        for (auto it = enemyManager->enemies.begin(); it != enemyManager->enemies.end();) {
            for (auto it2 = bulletManager->bullets.begin(); it2 != bulletManager->bullets.end();) {
                if ((it->circle).intersects(it2->circle) && it->kind <= 3) {
                    it->hp--;
                    it2 = bulletManager->bullets.erase(it2);
                    if (it->hp <= 0) {
                        //エフェクト
                        effect->add<Spark>(it->pos);
                        AudioAsset(U"explosion1").playOneShot();
                        hit = true;
                    }
                }
                else if ((it->cross).intersects(it2->circle) && it->kind == 4) {
                    it->hp--;
                    it2 = bulletManager->bullets.erase(it2);
                    if (it->hp <= 0) {
                        //エフェクト
                        effect->add<Spark>(it->pos);
                        AudioAsset(U"explosion3").playOneShot();
                        hit = true;
                    }
                }
                else if ((it->plus).intersects(it2->circle) && it->kind == 5) {
                    it->hp--;
                    it2 = bulletManager->bullets.erase(it2);
                    if (it->hp <= 0) {
                        //エフェクト
                        effect->add<Spark>(it->pos);
                        AudioAsset(U"explosion3").playOneShot();
                        hit = true;
                    }
                }
                else {
                    it2++;
                }
            }
            if (hit) {
                if (it->kind >= 4) {
                    *score += 100;
                    gameclear = true;
                }
                it = enemyManager->enemies.erase(it);
                *score += 1;
                *highscore = Max(*highscore, *score);
                enemySpawnTime *= 0.99;
                hit = false;
            }
            else {
                it++;
            }
        }
    }
    if (enemyManager->enemybullets.isEmpty()) {
        return;
    }
    //敵ショットと自機の衝突判定
    for (auto it = enemyManager->enemybullets.begin(); it != enemyManager->enemybullets.end();) {
        if (player->invincible) {
            break;
        }
        if (it->circle.intersects(player->circle)) {
            player->life -= 1;
            //エフェクト
            effect->add<Spark>(player->pos);
            AudioAsset(U"explosion2").playOneShot();
            player->invincible = true;
            if (player->life <= 0) {
                gameover = true;
            }
            break;
        }
        else {
            it++;
        }
    }
    return;
}

int32 stage = 1;

// ゲームシーン
class Game : public App::Scene {
public:
    Player player;
    EnemyManager enemyManager;
    BulletManager bulletManager;
    double enemySpawnTimer = 0; //敵の発生間隔タイマー
    Effect effect;
    bool bossAppear = false;
    bool isPause = false;
    ItemManager itemManager;
    double itemSpawnTimer = 0; //アイテムの発生間隔タイマー
    double itemSpawnTime = 10; //アイテムの発生間隔
    bool regi = false; //ランキングにスコアを登録したか

    // コンストラクタ（必ず実装）
    Game(const InitData& init)
        : IScene(init)
    {
        stopwatch.restart();
        AudioAsset(U"Main_BGM").setVolume(0.3);
        AudioAsset(U"Main_BGM").play();
    }

    void update() override
    {
        //Rキーを押したらリスタート
        if (KeyR.down()) {
            gameover = false;
            gameclear = false;
            getData().score = 0;
            enemySpawnTime = 2;
            enemyManager.enemies.clear();
            bulletManager.bullets.clear();
            enemyManager.enemybullets.clear();
            itemManager.items.clear();
            player.pos = Vec2(400, 300);
            stopwatch.restart();
            bossAppear = false;
            player.life = 3;
            AudioAsset(U"gameover_BGM").stop();
            AudioAsset(U"Main_BGM").setVolume(0.3);
            AudioAsset(U"Main_BGM").play();
        }
        //Tキーを押したらタイトルヘ
        if (KeyT.down()) {
            AudioAsset(U"Main_BGM").stop();
            changeScene(U"Title");
        }
        //スペースキーを押したらポーズ<->ポーズ解除
        if (KeySpace.down()) {
            isPause = !isPause;
        }
        if (isPause) {
            stopwatch.pause();
            return;
        }
        if (stopwatch.isPaused() && !gameclear && !gameover) {
            stopwatch.resume();
        }
        //ゲームオーバー
        if (gameover) {
            stopwatch.pause();
            AudioAsset(U"Main_BGM").stop();
            AudioAsset(U"gameover_BGM").setVolume(0.3);
            AudioAsset(U"gameover_BGM").play();
            if (KeyT.down()) {
                // 停止して再生位置を最初に戻す
                AudioAsset(U"gameover_BGM").stop();
                changeScene(U"Title");
            }
           
            if (KeyW.down()) {
                AudioAsset(U"gameover_BGM").stop();
                changeScene(U"ScoreRegister");
            }
            return;
        }

        //ゲームクリア
        if (gameclear) {
            enemyManager.enemies.clear();
            bulletManager.bullets.clear();
            enemyManager.enemybullets.clear();
            itemManager.items.clear();
            stopwatch.pause();
            AudioAsset(U"Main_BGM").stop();
            AudioAsset(U"clear_BGM").setVolume(0.3);
            AudioAsset(U"clear_BGM").play();
            if (KeyT.down()) {
                changeScene(U"Title");
            }
            if (KeyN.down()) {
                stage++;
                gameclear = false;
                enemyManager.enemies.clear();
                bulletManager.bullets.clear();
                enemyManager.enemybullets.clear();
                player.pos = Vec2(400, 300);
                stopwatch.restart();
                bossAppear = false;
                AudioAsset(U"gameover_BGM").stop();
                AudioAsset(U"Main_BGM").setVolume(0.3);
                AudioAsset(U"Main_BGM").play();
            }
        }

        //ボス出現
        if (stopwatch.sF() > 25 && !bossAppear) {
            if (stage == 1) {
                Enemy boss(Vec2(400, 100), 4, 10);
                boss.setPlayerPtr(&player);//ポインタを入れる
                enemyManager.add(boss);
            }
            if (stage == 2) {
                Enemy boss(Vec2(400, 100), 5, 15);
                boss.setPlayerPtr(&player);//ポインタを入れる
                enemyManager.add(boss);
            }
            bossAppear = true;
        }

        player.update();
        enemyManager.update();
        bulletManager.update();
        itemManager.update();
        CollisionDetection(&enemyManager, &bulletManager, &player, &effect, &getData().score, &getData().highscore);
        GetDetection(&itemManager, &player);
        enemySpawnTimer += Scene::DeltaTime();
        enemyShotTimer += Scene::DeltaTime();
        itemSpawnTimer += Scene::DeltaTime();
        if (player.invincible) {
            invincibleTimer += Scene::DeltaTime();
        }

        //無敵解除
        if (invincibleTimer > invincibleTime) {
            player.invincible = false;
            invincibleTimer = 0;
        }

        //アイテムの発生
        if (itemSpawnTimer > itemSpawnTime) {
            //ランダムな位置にアイテムを作成
            Item item(Vec2(Random(Scene::Width()), Random(Scene::Height())), 1);
            itemManager.add(item);
            itemSpawnTimer -= itemSpawnTime;
        }

        //敵の発生
        if (enemySpawnTimer > enemySpawnTime) {
            //ランダムな位置に敵を作成
            Enemy enemy(Vec2(Random(Scene::Width()), Random(Scene::Height())), Random(1, 3), 1);
            if (!enemy.circle.intersects(player.circle)) {
                enemy.setPlayerPtr(&player);//ポインタを入れる
                enemyManager.add(enemy);
                enemySpawnTimer -= enemySpawnTime;
            }
        }
        //自機ショットの発射
        if (KeyW.down()) {
            Bullet bullet(player.pos + Vec2(0, -30), 0);
            bulletManager.add(bullet);
        }
        if (KeyD.down()) {
            Bullet bullet(player.pos + Vec2(0, -30), 1);
            bulletManager.add(bullet);
        }
        if (KeyS.down()) {
            Bullet bullet(player.pos + Vec2(0, -30), 2);
            bulletManager.add(bullet);
        }
        if (KeyA.down()) {
            Bullet bullet(player.pos + Vec2(0, -30), 3);
            bulletManager.add(bullet);
        }
        effect.update();
    }

    void draw() const override
    {
        Scene::SetBackground(ColorF(0.3, 0.6, 1.0)); //背景色
        ClearPrint();
        //Print << U"enemies: " << enemyManager.enemies.size(); //現在の敵の数の表示
        Print << U"score: " << getData().score; //スコア表示
        Print << U"high score: " << getData().highscore; //ハイスコア表示
        Print << U"time: " << stopwatch.sF();
        Print << U"life: " << player.life; //残機表示
        //Print << U"enemy spawn time: " << enemySpawnTime;
        //Print << U"enemy shot timer: " << enemyShotTimer;
        //Print << U"item spawn timer: " << itemSpawnTimer;
        if (bossAppear) {
            Print << U"boss life: " << enemyManager.boss_hp();
        }

        //ゲームクリア
        if (gameclear) {
            FontAsset(U"BigFont")(U"Game Clear!").drawAt(400, 150);
            FontAsset(U"BigFont")(U"Press T to back to title").drawAt(400, 250);
            FontAsset(U"BigFont")(U"Press R to retry").drawAt(400, 350);
            FontAsset(U"BigFont")(U"Press N to go to\nnext stage").drawAt(400, 500);
            return;
        }

        player.draw();
        enemyManager.draw();
        bulletManager.draw();
        itemManager.draw();

        //ゲームオーバー
        if (gameover) {
            FontAsset(U"BigFont")(U"Game Over!").drawAt(400, 150);
            FontAsset(U"BigFont")(U"Press T to back to title").drawAt(400, 250);
            FontAsset(U"BigFont")(U"Press R to retry").drawAt(400, 350);
            FontAsset(U"BigFont")(U"Press W to register").drawAt(400, 450);
        }
    }
};

void Main()
{   
    FontAsset::Register(U"BigFont", 60, Typeface::Heavy);
    FontAsset::Register(U"ScoreFont", 30, Typeface::Bold);

    // シーンマネージャーを作成
    App manager;

    // シーンの登録
    manager.add<Title>(U"Title");
    manager.add<Game>(U"Game");
    manager.add<Ranking>(U"Ranking");
    manager.add<ScoreRegister>(U"ScoreRegister");


    // アセットの登録
    AudioAsset::Register(U"Main_BGM", Resource(U"RAIN_&_Co_II_2.mp3"));
    AudioAsset::Register(U"explosion1", Resource(U"small_explosion1.mp3"));
    AudioAsset::Register(U"explosion2", Resource(U"small_explosion2.mp3"));
    AudioAsset::Register(U"explosion3", Resource(U"explosion3.mp3"));
    AudioAsset::Register(U"gameover_BGM", Resource(U"Endless_Nightmare.mp3"));
    AudioAsset::Register(U"clear_BGM", Resource(U"jingle.mp3"));
    AudioAsset::Register(U"title_BGM", Resource(U"GI.mp3"));
    AudioAsset::Register(U"ranking_BGM", Resource(U"ranking_ranker.mp3"));

    TextureAsset::Register(U"heart", Emoji(U"💗"));

    AudioAsset(U"Main_BGM").setLoop(true);
    AudioAsset(U"gameover_BGM").setLoop(true);
    AudioAsset(U"title_BGM").setLoop(true);

    while (System::Update()) {
        // 現在のシーンを実行
        if (!manager.update())
        {
            break;
        }
    }

}

//
// = アドバイス =
// Debug ビルドではプログラムの最適化がオフになります。
// 実行速度が遅いと感じた場合は Release ビルドを試しましょう。
// アプリをリリースするときにも、Release ビルドにするのを忘れないように！
//
// 思ったように動作しない場合は「デバッグの開始」でプログラムを実行すると、
// 出力ウィンドウに詳細なログが表示されるので、エラーの原因を見つけやすくなります。
//
// = お役立ちリンク =
//
// OpenSiv3D リファレンス
// https://siv3d.github.io/ja-jp/
//
// チュートリアル
// https://siv3d.github.io/ja-jp/tutorial/basic/
//
// よくある間違い
// https://siv3d.github.io/ja-jp/articles/mistakes/
//
// サポートについて
// https://siv3d.github.io/ja-jp/support/support/
//
// Siv3D ユーザコミュニティ Slack への参加
// https://siv3d.github.io/ja-jp/community/community/
//
// 新機能の提案やバグの報告
// https://github.com/Siv3D/OpenSiv3D/issues
//
