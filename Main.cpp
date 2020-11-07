#include <Siv3D.hpp> // OpenSiv3D v0.4.3

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
    }

    // 更新関数
    void update() override
    {
        // 左クリックで
        if (MouseL.down())
        {
            // ゲームシーンに遷移
            changeScene(U"Game");
        }
    }

    // 描画関数 (const 修飾)
    void draw() const override
    {
        Scene::SetBackground(ColorF(0.3, 0.4, 0.5));

        FontAsset(U"BigFont")(U"Shooting Game").drawAt(400, 100);
        FontAsset(U"BigFont")(U"Click to start").drawAt(400, 300);
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

//敵
class Enemy {
public:
    Vec2 pos;
    Vec2 velocity; //敵の速度
    Player* pPlayer;
    Circle circle;
    int32 kind;
    int32 hp;

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
        if (kind == 1 || kind == 4) {
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
        pos += velocity;
        circle = Circle(pos, 30.0);
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
            Shape2D::Cross(80, 10, pos).draw(Palette::Skyblue);
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
                if (enemies[i].kind != 4) {
                    Vec2 vel = RandomVec2(5);
                    EnemyBullet enemybullet(enemies[i].pos + 6 * vel, vel);
                    enemybullets << enemybullet;
                }
                else {
                    for (auto j : step(4)) {
                        Vec2 vel(5 * cos(j * 90_deg + 45_deg), 5 * sin(j * 90_deg + 45_deg));
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
        if ((it->circle).intersects(player->circle)) {
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
                if ((it->circle).intersects(it2->circle)) {
                    it->hp--;
                    it2 = bulletManager->bullets.erase(it2);
                    if (it->hp <= 0) {
                        //エフェクト
                        effect->add<Spark>(it->pos);
                        if (it->kind <= 3) {
                            AudioAsset(U"explosion1").playOneShot();
                        }
                        else {
                            AudioAsset(U"explosion3").playOneShot();
                        }
                        hit = true;
                    }
                }
                else {
                    it2++;
                }
            }
            if (hit) {
                if (it->kind == 4) {
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
            return;
        }

        //ゲームクリア
        if (gameclear) {
            enemyManager.enemies.clear();
            bulletManager.bullets.clear();
            enemyManager.enemybullets.clear();
            stopwatch.pause();
            if (KeyT.down()) {
                // 停止して再生位置を最初に戻す
                AudioAsset(U"Main_BGM").stop();
                changeScene(U"Title");
            }
        }

        //ボス出現
        if (stopwatch.sF() > 25 && !bossAppear) {
            Enemy boss(Vec2(400, 100), 4, 10);
            boss.setPlayerPtr(&player);//ポインタを入れる
            enemyManager.add(boss);
            bossAppear = true;
        }

        player.update();
        enemyManager.update();
        bulletManager.update();
        CollisionDetection(&enemyManager, &bulletManager, &player, &effect, &getData().score, &getData().highscore);
        enemySpawnTimer += Scene::DeltaTime();
        enemyShotTimer += Scene::DeltaTime();
        if (player.invincible) {
            invincibleTimer += Scene::DeltaTime();
        }

        //無敵解除
        if (invincibleTimer > invincibleTime) {
            player.invincible = false;
            invincibleTimer = 0;
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
       

        //ゲームクリア
        if (gameclear) {
            FontAsset(U"BigFont")(U"Game Clear!").drawAt(400, 250);
            FontAsset(U"BigFont")(U"Press T to back to title").drawAt(400, 350);
            FontAsset(U"BigFont")(U"Press R to retry").drawAt(400, 450);
            return;
        }

        player.draw();
        enemyManager.draw();
        bulletManager.draw();

        //ゲームオーバー
        if (gameover) {
            FontAsset(U"BigFont")(U"Game Over!").drawAt(400, 250);
            FontAsset(U"BigFont")(U"Press T to back to title").drawAt(400, 350);
            FontAsset(U"BigFont")(U"Press R to retry").drawAt(400, 450);
        }
    }
};

void Main()
{   
    FontAsset::Register(U"BigFont", 60, Typeface::Heavy);
    FontAsset::Register(U"ScoreFont", 30, Typeface::Bold);

    // シーンマネージャーを作成
    App manager;

    // タイトルシーン（名前は U"Title"）を登録
    manager.add<Title>(U"Title");

    // ゲームシーン（名前は U"Game"）を登録
    manager.add<Game>(U"Game");

    // アセットの登録
    AudioAsset::Register(U"Main_BGM", U"RAIN_&_Co_II_2.mp3");
    AudioAsset::Register(U"explosion1", U"small_explosion1.mp3");
    AudioAsset::Register(U"explosion2", U"small_explosion2.mp3");
    AudioAsset::Register(U"explosion3", U"explosion3.mp3");
    AudioAsset::Register(U"gameover_BGM", U"Endless_Nightmare.mp3");

    AudioAsset(U"Main_BGM").setLoop(true);
    AudioAsset(U"gameover_BGM").setLoop(true);

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
