<?php
function perepel_karaokeru_import_form($form_state) {
    $form = array();
    $form['statistics'] = array(
        '#type' => 'fieldset',
        '#title' => t('Статистика импорта из Karaoke.ru'),
    );
    $form['statistics']['imported'] = array(
        '#type' => 'item',
        '#title' => 'Импортировано песен',
        '#markup' => PerepelKaraokeru::getImportedCount(),
    );
    $lastUpdateTime = variable_get(PKR_LAST_UPDATE_TIME, 0);
    $lastUpdateExpiredTime = variable_get(PKR_LAST_UPDATE_EXPIRED_TIME, 0);
    $form['statistics']['updated'] = array(
        '#type' => 'item',
        '#title' => 'Количество необработанных обновлений',
        '#markup' => PerepelKaraokeru::getUpdatedCount() . ($lastUpdateTime ? sprintf(' (с %s)', date('d.m.Y H:i:s', $lastUpdateTime)) : ''),
    );

    $form['statistics']['outdated'] = array(
        '#type' => 'item',
        '#title' => 'Количество песен с истекшими правами',
        '#markup' => PerepelKaraokeru::getListExpired() . ($lastUpdateExpiredTime ? sprintf(' (с %s)', date('d.m.Y H:i:s', $lastUpdateExpiredTime)) : ''),
    );

    if (variable_get(PKR_CRON_ENABLED, 0)) {
        $lastRun = variable_get(PKR_CRON_LAST_RUN, 0);
        $lastProcessed = variable_get(PKR_CRON_LAST_UPDATES_PROCESSED, 0);
        $form['statistics']['cron_info'] = array(
            '#type' => 'item',
            '#markup' => ($lastRun > 0 ? '<i>Последняя автоматическая обработка обновлений выполнена '
                . date('d.m.Y H:i:s', $lastRun) . '. Обработано '.$lastProcessed
                . perepel_word_by_number($lastProcessed, ' обновление', ' обновления', ' обновлений') . '.</i>'
                : '<i>Включена автоматическая обработка обновлений. Пока ни разу не выполнялась.</i>'),
        );
    } else
        $form['statistics']['cron_info'] = array(
            '#type' => 'item',
            '#markup' => '<i>Автоматическая обработка обновлений отключена.</i>',
        );
    $updatingStarted = PerepelKaraokeru::updatingStarted();
    $form['updating'] = array(
        '#type' => 'fieldset',
        '#title' => 'Ручная загрузка обновлений',
    );
    if ($updatingStarted)
        $form['updating']['run_info'] = array(
            '#type' => 'item',
            '#markup' => '<div class="messages warning">Внимание! В настоящий момент выполняется обработка обновлений.</div>',
        );
    $form['updating']['update_limit'] = array(
        '#type' => 'textfield',
        '#title' => 'Количество обновлений, обрабатываемых за одну операцию',
        '#description' => 'Значение от 1 до 1000',
        '#default_value' => variable_get(PKR_UPDATE_LIMIT, 200),
    );
    $form['updating']['update'] = array(
        '#type' => 'submit',
        '#value' => 'Загрузить обновления',
        '#validate' => array('perepel_karaokeru_update_validate'),
        '#submit' => array('perepel_karaokeru_update_submit'),
        '#disabled' => $updatingStarted,
    );
    $form['updating']['catalogue'] = array(
        '#type' => 'submit',
        '#value' => 'Получить список обновлений',
        '#submit' =>array('perepel_karaokeru_catalogue_submit'),
    );
    $form['updating']['check_expire'] = array(
        '#type' => 'submit',
        '#value' => 'Проверка истекших песен',
        '#submit' =>array('perepel_karaokeru_check_expire_submit'),
    );
    $form['updating']['total'] = array(
        '#type' => 'submit',
        '#value' => 'Полное обновление каталога',
        '#submit' =>array('perepel_karaokeru_total_submit'),
    );
    $form['updating']['categories'] = array(
        '#type' => 'submit',
        '#value' => 'Загрузить категории',
        '#submit' =>array('perepel_karaokeru_categories_submit'),
    );
    $form['newlogic'] = array(
        '#type' => 'fieldset',
        '#title' => 'Новые механизмы обновления',
    );
    $form['newlogic']['test'] = array(
        '#type' => 'submit',
        '#value' => 'Test',
        '#submit' =>array('perepel_karaokeru_artists_submit'),
    );

    $form['newlogic']['testcron'] = array(
        '#type' => 'submit',
        '#value' => 'Schedule artist update via cron',
        '#submit' =>array('perepel_karaokeru_artistscron_submit'),
    );

    $form['newlogic']['newcatupdate'] = array(
        '#type' => 'submit',
        '#value' => 'Get new songs via cron',
        '#submit' =>array('perepel_karaokeru_newupdate_submit'),
    );
    $form['newlogic']['expireupdate'] = array(
        '#type' => 'submit',
        '#value' => 'Update expired songs',
        '#submit' =>array('perepel_karaokeru_expireupdate_submit'),
    );
    $form['newlogic']['testupdate'] = array(
        '#type' => 'submit',
        '#value' => 'Manual song update',
        '#submit' =>array('perepel_karaokeru_testupdate_submit'),
    );
    $form['newlogic']['expired_start'] = array(
        '#type' => 'submit',
        '#value' => 'Test: start expired',
        '#submit' =>array('perepel_karaokeru_expired_submit'),
    );


    return $form;
}


function perepel_karaokeru_config_form($form_state) {
    $form['cron'] = array(
        '#type' => 'fieldset',
        '#title' => 'Автоматическая синхронизация песен',
    );
    $lastRun = variable_get(PKR_CRON_LAST_RUN, 0);
    $lastProcessed = variable_get(PKR_CRON_LAST_UPDATES_PROCESSED, 0);
    if ($lastRun > 0)
        $form['cron']['last_run'] = array(
            '#type' => 'item',
            '#markup' => '<i>Последняя автоматическая обработка обновлений выполнена '
                . date('d.m.Y H:i:s', $lastRun) . '. Обработано '.$lastProcessed
                . perepel_word_by_number($lastProcessed, ' обновление', ' обновления', ' обновлений') . '.</i>',
        );
    try {
        $jobEnabled = !(db_select('elysia_cron', 'ec')
            ->fields('ec', array('disable'))
            ->condition('name', 'update_songs')
            ->execute()
            ->fetchField());
    } catch (Exception $e) {
        $jobEnabled = true;
    }
    $form['cron'][PKR_CRON_ENABLED] = array(
        '#type' => 'checkbox',
        '#title' => 'Включить автоматическую обработку обновлений (частота обработки определяется настройкой задания update_songs в cron)',
        '#default_value' => $jobEnabled,
    );
    $form['cron'][PKR_CRON_LOG_MESSAGES] = array(
        '#type' => 'checkbox',
        '#title' => 'Протоколировать выполнение обработки (watchdog)',
        '#default_value' => variable_get(PKR_CRON_LOG_MESSAGES, 0),
        '#states' => array(
            'visible' => array('input[name="'.PKR_CRON_ENABLED.'"]' => array('checked' => true)),
        ),
    );
    $form['cron'][PKR_CRON_UPDATE_LIMIT] = array(
        '#type' => 'textfield',
        '#title' => 'Количество обновлений, загружаемых за один запуск задания',
        '#description' => 'Значение от 10 до 500. Рекомендуемые значения: 100-200.',
        '#default_value' => variable_get(PKR_CRON_UPDATE_LIMIT, 200),
        '#states' => array(
            'visible' => array('input[name="'.PKR_CRON_ENABLED.'"]' => array('checked' => true)),
        ),
    );
    $form['cron'][PKR_CRON_TIME_LIMIT] = array(
        '#type' => 'textfield',
        '#title' => 'Лимит времени на обработку порции обновлений',
        '#description' => 'Значение в секундах от 20 до 60. Рекомендуемые значения: 30-40.',
        '#default_value' => variable_get(PKR_CRON_TIME_LIMIT, 40),
        '#states' => array(
            'visible' => array('input[name="'.PKR_CRON_ENABLED.'"]' => array('checked' => true)),
        ),
    );
    return system_settings_form($form, true);
}

function perepel_karaokeru_update_validate($form, &$form_state) {
    $limit = intval($form_state['values']['update_limit']);
    if ($limit < 1 || $limit > 1000)
        form_set_error('update_limit', 'Некорректное количество обновлений, обрабатываемых за одну операцию.');
}


function perepel_karaokeru_update_submit($form, &$form_state) {
    if (!PerepelKaraokeru::getLock(10)) {
        drupal_set_message('Обработка обновлений уже запущена другим процессом.', 'warning');
        return $form;
    }
    $limit = intval($form_state['values']['update_limit']);
    $songs = PerepelKaraokeru::getUpdatedSongs($limit);
    if (empty($songs)) {
        drupal_set_message('Обновлений не обнаружено.');
        PerepelKaraokeru::releaseLock();
        return $form;
    }
    $operations = array();
    $batchSongs = array();
    $i = 0;
    foreach ($songs as $song) {
        $batchSongs[] = $song;
        $i++;
        if ($i % PerepelKaraokeru::getBatchUpdateLimit() == 0) {
            $operations[] = array('perepel_karaokeru_batch_update', array(array('songs' => $batchSongs)));
            $batchSongs = array();
        }
    }
    if (!empty($batchSongs))
        $operations[] = array('perepel_karaokeru_batch_update', array(array('songs' => $batchSongs)));
    $batch = array(
        'title' => 'Обработка обновлений',
        'init_message' => 'Подготовка данных',
        'operations' => $operations,
        'finished' => 'perepel_karaokeru_batch_update_finished',
        'file' => drupal_get_path('module', 'perepel_karaokeru') . '/perepel_karaokeru.importer.inc',
    );
    batch_set($batch);
}

function perepel_karaokeru_batch_update($data, &$context) {
    variable_set(PKR_LOCK_EXPIRES, time()+60);
    $songs = $data['songs'];
    if (!isset($context['results']['processed'])) $context['results']['processed'] = 0;
    if (!isset($context['results']['updated'])) $context['results']['updated'] = 0;
    if (!isset($context['results']['added'])) $context['results']['added'] = 0;
    if (!isset($context['results']['errors'])) $context['results']['errors'] = array();
    foreach ($songs as $song) {
        try {
            $updateResult = PerepelKaraokeru::processSongUpdate($song);
            if ($updateResult == PerepelKaraokeru::urUpdated)
                $context['results']['updated']++;
            elseif ($updateResult == PerepelKaraokeru::urAdded)
                $context['results']['added']++;
        } catch (Exception $e) {
            watchdog('perepel_karaokeru', $e->getMessage(), array(), WATCHDOG_ERROR);
            $context['results']['errors'][] = $e->getMessage();
        }
        variable_set(PKR_LAST_UPDATE_TIME, $song['updated_at']);
        variable_set(PKR_LAST_UPDATE_ID, $song['id']);
        $context['results']['processed']++;
    }
}

function perepel_karaokeru_batch_update_finished($success, $results, $operations) {
    PerepelKaraokeru::releaseLock();
    if ($success) {
        $processedCount = isset($results['processed']) ? $results['processed'] : 0;
        if ($processedCount > 0) {
            $successMessage = perepel_word_by_number($processedCount, 'Обработано ', 'Обработано ', 'Обработано ')
                . $processedCount . perepel_word_by_number($processedCount, ' обновление.', ' обновления.', ' обновлений.');
            $updatedCount = isset($results['updated']) ? $results['updated'] : 0;
            $addedCount = isset($results['added']) ? $results['added'] : 0;
            if ($updatedCount) $successMessage .= sprintf(' Изменено песен: %d.', $updatedCount);
            if ($addedCount) $successMessage .= sprintf(' Добавлено песен: %d.', $addedCount);
            if (!$addedCount && !$updatedCount) $successMessage .= ' Изменений не обнаружено.';
        } else {
            $successMessage = 'Обновлений не обнаружено.';
        }
        if (!empty($results['errors'])) {
            $successMessage .= sprintf(' Ошибок обработки обновлений: %d.', count($results['errors']));
            foreach ($results['errors'] as $error)
                drupal_set_message($error, 'error');
        } elseif ($processedCount > 0)
            $successMessage .= ' Ошибок обработки обновлений нет.';
        drupal_set_message($successMessage);
    } else {
        drupal_set_message(t('An error occurred while processing'), 'error');
    }
    unset($results['errors']);
}


function perepel_karaokeru_total_submit($form, &$form_state) {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $songs = PerepelKaraokeru::getCatalogue($sessionID, $url);
    $songs2update = $songs[0]['Songs'];

    if (empty($songs2update)) {
        drupal_set_message('Обновлений не обнаружено.');
        PerepelKaraokeru::releaseLock();
        return $form;
    }
    $operations = array();
    $batchSongs = array();
    $i = 0;
    foreach ($songs2update as $song) {
        $batchSongs[] = $song;
        $i++;
        if ($i % PerepelKaraokeru::getBatchUpdateLimit() == 0) {
            $operations[] = array('perepel_karaokeru_batch_total', array(array('songs' => $batchSongs)));
            $batchSongs = array();
        }
    }
    if (!empty($batchSongs))
        $operations[] = array('perepel_karaokeru_batch_total', array(array('songs' => $batchSongs)));
    $batch = array(
        'title' => 'Обработка обновлений',
        'init_message' => 'Подготовка данных',
        'operations' => $operations,
        'finished' => 'perepel_karaokeru_batch_total_finished',
        'file' => drupal_get_path('module', 'perepel_karaokeru') . '/perepel_karaokeru.importer.inc',
    );
    batch_set($batch);
}


function perepel_karaokeru_newupdate_submit($form, &$form_state) {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $catalogue = PerepelKaraokeru::getCatalogue($sessionID, $url);
    $songs2update = PerepelKaraokeru::getListUpdate($catalogue,$sessionID, $url,100000);

    if (empty($songs2update)) {
        drupal_set_message('Обновлений не обнаружено.');
        PerepelKaraokeru::releaseLock();
        return $form;
    }

    $queue = DrupalQueue::get('perepel_karaokeru_catalogue');

    $operations = array();
    $batchSongs = array();
    $i = 0;
    foreach ($songs2update as $song) {
        $batchSongs[] = $song;
        $i++;
        if ($i % 10 == 0) {
            $operations = $batchSongs;
            $queue->createItem($operations);
            $batchSongs = array();
        }
    }
    if (!empty($batchSongs)) {
        $operations = $batchSongs;
        $queue->createItem($operations);
    }

}

function perepel_karaokeru_expired_submit($form, &$form_state) {
    $queue = DrupalQueue::get('perepel_karaokeru_expired_update');

    $operations = array();
    $batchSongs = array();
    $batchSongs[] = 1;
    $operations = $batchSongs;
    $queue->createItem($operations);
    $batchSongs = array();
}

function perepel_karaokeru_artists_submit ($form, &$form_state) {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $songs = PerepelKaraokeru::getCatalogue($sessionID, $url);
    $songs2update = $songs[0]['Songs'];

    $operations = array();
    $batchSongs = array();
    $i = 0;
    foreach ($songs2update as $song) {
        $batchSongs[] = $song;
        $i++;
        if ($i == 1) {
            $operations[] = array('perepel_karaokeru_batch_artists_worker', array(array('songs' => $batchSongs)));
            $batchSongs = array();
        }
    }
    if (!empty($batchSongs))
        $operations[] = array('perepel_karaokeru_batch_artists_worker', array(array('songs' => $batchSongs)));
    $batch = array(
        'title' => 'Обработка исполнителей',
        'init_message' => 'Подготовка данных',
        'operations' => $operations,
        'finished' => 'perepel_karaokeru_batch_artists_finished',
        'file' => drupal_get_path('module', 'perepel_karaokeru') . '/perepel_karaokeru.importer.inc',
    );
    batch_set($batch);
}

function perepel_karaokeru_artistscron_submit ($form, &$form_state) {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $songs = PerepelKaraokeru::getCatalogue($sessionID, $url);
    $songs2update = $songs[0]['Songs'];

    $queue = DrupalQueue::get('perepel_karaokeru_artists');

    $operations = array();
    $batchSongs = array();
    $i = 0;
    foreach ($songs2update as $song) {
        $batchSongs[] = $song;
        $i++;
        if ($i % 50 == 0) {
            $operations = $batchSongs;
            $queue->createItem($operations);
            $batchSongs = array();
        }
    }
    if (!empty($batchSongs))
        $operations = array('perepel_karaokeru_batch_artists_worker', array(array('songs' => $batchSongs)));
    $queue->createItem($operations);
}

function perepel_karaokeru_expireupdate_submit () {
    $checkSongs = db_select ('karaokeru_songs_20', 'a')
        ->fields ('a', array ('nid'))
        ->condition ('karaoke_id', 97661, '=');
    $checkSong = $checkSongs->execute()->fetchField();
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $songs = db_select ('karaokeru_songs_20', 'a')
        ->fields ('a',array('karaoke_id'))
        ->condition ('end_date', time(), '<');
    $expired = $songs->execute()->fetchAll();
    foreach ($expired as $song) {
        $object = PerepelKaraokeru::getSongDataNew($song->karaoke_id, $sessionID);
        $object = json_decode($object,true);

        if ($object[0]['expire_at'] == 0) {
            try {
                $update = db_update ('karaokeru_songs_20')
                    ->fields(array(
                        'end_date' => 0,
                        'karaoke_updated_at' => strtotime($object[0]['updated_at']),
                        'sync_date' => time(),
                    ))
                    ->condition('karaoke_id', $song->karaoke_id, '=')
                    ->execute();
            }
            catch (Exception $e) {

            }
        }

        if (strtotime($object[0]['expire_at']) > time()) {

            $update = db_update ('karaokeru_songs_20')
                ->fields(array(
                    'end_date' => (int) strtotime($object[0]['expire_at']),
                    'karaoke_updated_at' => (int) strtotime($object[0]['updated_at']),
                    'sync_date' => time(),
                ))
                ->condition('karaoke_id', $song->karaoke_id, '=')
                ->execute();
        }
    }
    variable_set(PKR_LAST_UPDATE_EXPIRED_TIME, time());
    variable_set('perepel_karaokeru_last_update_expired_time', time());
}

function perepel_karaokeru_testupdate_submit () {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $catalogue = PerepelKaraokeru::getCatalogue($sessionID, $url);
    $songs2update = PerepelKaraokeru::getListUpdate($catalogue,$sessionID, $url,10);
    $object = PerepelKaraokeru::getSongDataNew($songs2update, $sessionID);
    $dataUpdate = PerepelKaraokeru::getNewSong($object);
    if ($dataUpdate == 2 && !empty($dataUpdate)) {
        $importResult = PerepelKaraokeru::importNewSong ($dataUpdate);
    }
    return TRUE;
}

function perepel_karaokeru_categories_submit($form, &$form_state) {
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $categories = PerepelKaraokeru::getCategories($sessionID, $url);
    $catIndex = array();
    $cats = array();
    $i = 0;
    if ($categories) {
        array_push ($catIndex, $categories[0]['Collections']);
        $catIndex = $catIndex[0];
        foreach ($categories as $row1) {
            if (array_key_exists("_class", $row1)) {
                $catLoaded = db_select ('karaoke_categories','kc')
                    ->fields('kc', array('index'))
                    ->condition ('index',$row1["_self"],'=')
                    ->execute()->fetchField();
                if ($row1["_class"] == 'CollectionModel') {
                    if (empty($catLoaded)) {
                        $data = PerepelKaraokeru::getContent($row1['image_url']);
                        $icon = PerepelKaraokeru::getContent($row1['icon_url']);
                        if ($data) {
                            $filename = 'cat-' . $row1["_self"] . '.jpg';
                            $filename1 = 'cat-' . $row1["_self"] . '.png';
                            $dir = file_default_scheme() . '://catalogue/';
                            $file = file_save_data($data, $dir . $filename);
                            $file->display = 1;
                            $file1 = file_save_data($icon, $dir . $filename1);
                            $file1->display = 1;
                        }
                        $cats = array(
                            'index' => $row1["_self"],
                            'name' => $row1["title"],
                            'image' => $file->fid,
                            'icon' =>$file1->fid,
                            'updated' => time());
                        db_insert('karaoke_categories')->fields($cats)->execute();
                        $songs = array ();
                        $songs = $row1['Songs'];
                        $collections = array();
                        foreach ($songs as $song) {
                            $songID = str_replace('sng-','',$song);
                            $songNid = db_select ('karaokeru_songs_20','ks20')
                                ->fields('ks20', array('nid'))
                                ->condition ('karaoke_id',$songID,'=')
                                ->execute()->fetchField();
                            if (empty($songNid)) $songNid = 0;
                            $collections = array (
                                'index' => $row1["_self"],
                                'song' => $song,
                                'nid' => $songNid);

                            $checkCat = db_select ('karaoke_cat_songs', 'k');
                            $checkCat->condition ('index', $row1["_self"]);
                            $checkCat->condition ('song', $song);
                            $checkCat->condition ('nid', $songNid);
                            $checkCatResult = $checkCat->execute()->fetchField();

                            if (empty ($checkCatResult)) {
                                db_insert('karaoke_cat_songs')->fields($collections)->execute();
                            }
                        }
                    }
                }
            }
        }
    }
}

function perepel_karaokeru_batch_total($data, &$context) {
    variable_set(PKR_LOCK_EXPIRES, time()+60);
    $songs = $data['songs'];
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    if (!isset($context['results']['processed'])) $context['results']['processed'] = 0;
    if (!isset($context['results']['updated'])) $context['results']['updated'] = 0;
    if (!isset($context['results']['added'])) $context['results']['added'] = 0;
    if (!isset($context['results']['errors'])) $context['results']['errors'] = array();
    $i = 0;
    foreach ($songs as $song) {
        $i++;
        try {
            $broadData = PerepelKaraokeru::getSongData($song, $url, $sessionID);
            $dataUpdate = PerepelKaraokeru::checkSongStatus($broadData);
            $updateResult = PerepelKaraokeru::processSongUpdate($dataUpdate);
            if ($updateResult == PerepelKaraokeru::urUpdated)
                $context['results']['updated']++;
            elseif ($updateResult == PerepelKaraokeru::urAdded)
                $context['results']['added']++;
        } catch (Exception $e) {
            watchdog('perepel_karaokeru', $e->getMessage(), array(), WATCHDOG_ERROR);
            $context['results']['errors'][] = $e->getMessage();
        }
        variable_set(PKR_LAST_UPDATE_TIME, $dataUpdate['updated_at']);
        variable_set(PKR_LAST_UPDATE_ID, $dataUpdate['id']);
        $context['results']['processed']++;
    }
}


function perepel_karaokeru_check_expire_submit () {

    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;

    $query = db_select ('karaokeru_songs_20', 'a')
        ->fields ('a',array('karaoke_id'))
        ->condition ('end_date', time(), '>');
    $karaokeID = $query->execute()->fetchAll();

    foreach ($karaokeID as $row) {

        $check = PerepelKaraokeru::getSongData('sng-'. $row->karaoke_id, $url, $sessionID);

        $check = json_decode ($check, TRUE);
        $check = $check[0];

        if ($check['can_play'] != TRUE ) {

            $fields = array (
                'end_date' => time() - 2592000,
                'sync_date' => time(),
            );

            db_update ('karaokeru_songs_20')
                ->fields ($fields)
                ->condition ('karaoke_id', $row->karaoke_id)
                ->execute();

        }
    }

}

function perepel_karaokeru_batch_total_finished($success, $results, $operations) {
    PerepelKaraokeru::releaseLock();
    if ($success) {
        $processedCount = isset($results['processed']) ? $results['processed'] : 0;
        if ($processedCount > 0) {
            $successMessage = perepel_word_by_number($processedCount, 'Обработано ', 'Обработано ', 'Обработано ')
                . $processedCount . perepel_word_by_number($processedCount, ' обновление.', ' обновления.', ' обновлений.');
            $updatedCount = isset($results['updated']) ? $results['updated'] : 0;
            $addedCount = isset($results['added']) ? $results['added'] : 0;
            if ($updatedCount) $successMessage .= sprintf(' Изменено песен: %d.', $updatedCount);
            if ($addedCount) $successMessage .= sprintf(' Добавлено песен: %d.', $addedCount);
            if (!$addedCount && !$updatedCount) $successMessage .= ' Изменений не обнаружено.';
        } else {
            $successMessage = 'Обновлений не обнаружено.';
        }
        if (!empty($results['errors'])) {
            $successMessage .= sprintf(' Ошибок обработки обновлений: %d.', count($results['errors']));
            foreach ($results['errors'] as $error)
                drupal_set_message($error, 'error');
        } elseif ($processedCount > 0)
            $successMessage .= ' Ошибок обработки обновлений нет.';
        drupal_set_message($successMessage);
    } else {
        drupal_set_message(t('An error occurred while processing'), 'error');
    }
    unset($results['errors']);
}

function perepel_karaokeru_batch_artists_worker ($data, &$context) {
    variable_set(PKR_LOCK_EXPIRES, time()+60);
    if (empty($context['sandbox']['progress'])) {
        $context['sandbox']['progress'] = 0;
    }
    if (!isset($context['results']['processed'])) $context['results']['processed'] = 0;
    if (!isset($context['results']['updated'])) $context['results']['updated'] = 0;
    if (!isset($context['results']['added'])) $context['results']['added'] = 0;
    if (!isset($context['results']['errors'])) $context['results']['errors'] = array();
    $sessionID = PerepelKaraokeru::initConnection();
    $url = PerepelKaraokeru::$_url;
    $songs = $data['songs'];
    $i = 0;
    foreach ($songs as $song) {
        $i++;
        $object = PerepelKaraokeru::getSongDataNEW($song, $sessionID);
        $populate = PerepelKaraokeru::populateAuthors($object);
        $context['results']['processed']++;
    }
    $context['finished'] = 0;

}


function perepel_karaokeru_batch_artists_finished ($success, $results, $operations) {
    PerepelKaraokeru::releaseLock();
    if ($success) {
        $successMessage .= 'Информация об исполнителях обработана'; }
    else {
        drupal_set_message(t('An error occurred while processing'), 'error');
    }
    unset($results['errors']);
    drupal_set_message(t('@count products added.'));
}

class PerepelKaraokeru {

    public static function getImportedCount() {
        try {
            $result = db_select('karaokeru_songs_20', 'ks')
                ->countQuery()
                ->execute()
                ->fetchField();
        } catch (Exception $e) {
            $result = 'н/д';
        }
        return $result;
    }

    public static function getUpdatedCount() {
        $lastUpdateTime = variable_get(PKR_LAST_UPDATE_TIME, 0);
        $lastUpdatedSongId = variable_get(PKR_LAST_UPDATE_ID, 0);
        $url = "*****************";
        $limit = 100000;
        $sessionID = self::initConnection();
        try {
            $catalogue = self::getCatalogue($sessionID, $url);
            $list_update = self::getListUpdate($catalogue, $sessionID, $url,$limit);
            $result = count($list_update);
        } catch (Exception $e) {
            $result = 'н/д';
        }
        return $result;
    }

    public static function getCatalogue($sessionID, $url){
        $reqid = mt_rand();
        if (!$sessionID) PerepelKaraokeru::initConnection();
        $data_catalogue = array(
            "data" => array(
                "_reqid" => $reqid,
                "_request" => "Song_GetAllRequest",
                "Session" => $sessionID,
            ));
        $data = json_encode($data_catalogue);
        $ch = curl_init($url);

        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        $result = curl_exec($ch);
        curl_close($ch);  // Seems like good practice
        $json = json_decode($result, true);
        return $json;
    }

    public static function getCategories($sessionID, $url){
        $reqid = mt_rand();
        if (!$sessionID) PerepelKaraokeru::initConnection();
        $data_categories = array(
            "data" => array(
                "_reqid" => $reqid,
                "_request" => "Collection_GetListRequest",
                "Session" => $sessionID,
            ));
        $data = json_encode($data_categories);
        $ch = curl_init($url);

        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        $result = curl_exec($ch);
        curl_close($ch);  // Seems like good practice
        $json = json_decode($result, true);
        return $json;
    }

    private static function getUpdateData($catalogue, $sessionID, $url, $limit) {
        $list = false;
        $i = 0;
        $reqid = mt_rand();
        $catalogue = $catalogue[0]['Songs'];

        $sngArray = db_select('karaokeru_service','ksvc')
            ->fields('ksvc', array('song_id'))
            ->execute()->fetch();

        foreach ($catalogue as $row) {
            if (!in_array($row, $sngArray, TRUE)) {

                $list[] = array(
                    'id' => $row,
                    'updated_at' => time(),
                );
            }
            if (++$i == $limit) break;
        }
        return $list;
    }


    public static function getUpdatedSongs($limit) {
        // 1. Загружаем первые $limit обновленных песен, предварительно упорядочив их по updated_at и id:
        $url = "*****************";
        $sessionID = self::initConnection();
        $catalogue = self::GetCatalogue($sessionID, $url);
        $catalogue = $catalogue[0]['Songs'];
        $result = db_select('karaokeru_service', 'ks')
            ->countQuery()
            ->execute()
            ->fetchField();
        $catalogue = array_slice($catalogue, $result, 100000);
        $ids = array();
        $i = 0;
        foreach ($catalogue as $row) {
            $songData = self::getSongData($row, $url, $sessionID);
            $selector = count($songData) - 1;
            $authors = self::ProcessAuthors($songData);
            $updateTime = strtotime($songData[$selector]['updated_at']);
            $query = db_select('karaokeru_service','ksvc')
                ->fields('ksvc', array('updated_at'))
                ->condition('id', $songData[$selector]['id'], '=')
                ->execute()
                ->fetchfield();
            if (!empty($query)) {
                if ($query <= $updateTime) {
                    $fields = array (
                        'updated_at' => strtotime($songData[$selector]['updated_at']));
                    db_update('karaokeru_service')->fields($fields)->condition('id', $songData[$selector]['id'], '=')->execute();
                    $songs[] = array(
                        'id' => $songData[$selector]['id'],
                        'name' => $songData[$selector]['title'],
                        'end_date' => strtotime($songData[$selector]['expire_at']),
                        'updated_at' => $updateTime,
                        'authors' => array($authors),
                        'mp3' => $songData[$selector]['mp3'],
                        'dat' => $songData[$selector]['dat'],
                        'license' => $songData[$selector]['copyrights'],
                        'action' => null,
                    );

                    $ids[] = $songs[0]['id'];
                }}
            else {
                $songs[] = array(
                    'id' => $songData[$selector]['id'],
                    'name' => $songData[$selector]['title'],
                    'end_date' => strtotime($songData[$selector]['expire_at']),
                    'updated_at' => strtotime($songData[$selector]['updated_at']),
                    'authors' => array($authors),
                    'mp3' => $songData[$selector]['mp3'],
                    'dat' => $songData[$selector]['dat'],
                    'license' => $songData[$selector]['copyrights'],
                    'action' => null,
                );

                $ids[] = $songs[0]['id'];
            }

            if (++$i == $limit) break;
        }
        if (!empty($songs)) {
            // 2. Загружаем соответствующие записи perepel_karaokeru_songs:
            $query = db_select('karaokeru_songs_20', 'ks')
                ->fields('ks', array('nid', 'karaoke_id', 'end_date', 'karaoke_updated_at'))
                ->condition('karaoke_id', $ids, 'IN')
                ->execute();
            $rows = array();
            foreach ($query as $row) {

                $rows[$row->karaoke_id] = array(
                    'nid' => $row->nid,
                    'karaoke_id' => $row->karaoke_id,
                    'end_date' => $row->end_date,
                    'karaoke_updated_at' => $row->karaoke_updated_at,
                );
            }
            // 3. Сопоставляем данные из karaoke.ru с записями perepel_karaokeru_songs, определяем действия:
            $ids = array();
            foreach ($songs as $key => $song) {
                if (isset($rows[$song['id']])) {
                    if ($rows[$song['id']]['karaoke_updated_at'] >= $song['updated_at'])
                        $songs[$key]['action'] = 'update';
                    else {
                        $songs[$key]['action'] = 'update';
                        $songs[$key]['perepel_data'] = $rows[$song['id']];
                        $ids[] = $song['id'];

                    }
                } else {
                    if (is_numeric($song['end_date']) && $song['end_date'] <= REQUEST_TIME)
                        $songs[$key]['action'] = 'update';
                    else {
                        $songs[$key]['action'] = 'add';
                        $ids[] = $song['id'];
                    }
                }
            }
            // 4. Возвращаем песни:
            return $songs;
        }
    }


    public static function checkSongStatus($incomingData) {
        // 1. Ищем песню в списке загруженных
        $dataUpdate = array();
        $incomingData = json_decode($incomingData, true);
        $selector = count($incomingData) - 1;
        $query = db_select('karaokeru_songs_20','ks20')
            ->fields('ks20', array('nid','karaoke_id','end_date','karaoke_updated_at'))
            ->condition('karaoke_id', $incomingData[$selector]['id'], '=')
            ->execute()
            ->fetchAll();
        if (empty($query)) {
            $authors = self::ProcessAuthors($incomingData);
            $updateTime = strtotime($incomingData[$selector]['updated_at']);
            $dataUpdate = array(
                'id' => $incomingData[$selector]['id'],
                'name' => $incomingData[$selector]['title'],
                'end_date' => strtotime($incomingData[$selector]['expire_at']),
                'updated_at' => $updateTime,
                'authors' => array($authors),
                'mp3' => $incomingData[$selector]['mp3'],
                'dat' => $incomingData[$selector]['dat'],
                'license' => $incomingData[$selector]['copyrights'],
                'action' => 'add',
            );
        }
        elseif (!empty($query)) {
            $query = $query[0];
            $node = node_load($query->nid);
            if ($query->end_date < strtotime($incomingData[$selector]['expire_at'])) {
                $authors = self::ProcessAuthors($incomingData);
                $updateTime = strtotime($incomingData[$selector]['updated_at']);
                $dataUpdate = array(
                    'id' => $incomingData[$selector]['id'],
                    'name' => $incomingData[$selector]['title'],
                    'end_date' => strtotime($incomingData[$selector]['expire_at']),
                    'updated_at' => $updateTime,
                    'authors' => array($authors),
                    'mp3' => $incomingData[$selector]['mp3'],
                    'dat' => $incomingData[$selector]['dat'],
                    'license' => $incomingData[$selector]['copyrights'],
                    'action' => 'update',
                    'perepel_data' => array (
                        'nid' => $query->nid,
                        'karaoke_id' => $query->karaoke_id,
                        'end_date' => $query->end_date,
                        'karaoke_updated_at' => $query->karaoke_updated_at,
                    ),
                );
            }
            else {
                $mLink = $node->field_minus[LANGUAGE_NONE][0]['uri'];
                $sLink = $node->field_scenario[LANGUAGE_NONE][0]['uri'];
                $pLink = $node->field_photo_ipad[LANGUAGE_NONE][0]['uri'];
                $authors = self::ProcessAuthors($incomingData);
                $updateTime = strtotime($incomingData[$selector]['updated_at']);
                $dataUpdate = array(
                    'id' => $incomingData[$selector]['id'],
                    'name' => $incomingData[$selector]['title'],
                    'end_date' => strtotime($incomingData[$selector]['expire_at']),
                    'updated_at' => $updateTime,
                    'authors' => array($authors),
                    'mp3' => $incomingData[$selector]['mp3'],
                    'dat' => $incomingData[$selector]['dat'],
                    'license' => $incomingData[$selector]['copyrights'],
                );
                if (!file_exists($mLink) || !file_exists($sLink) || !file_exists($pLink)) {
                    $fields = array(
                        'action' => 'update',
                        'perepel_data' => array (
                            'nid' => $query->nid,
                            'karaoke_id' => $query->karaoke_id,
                            'end_date' => $query->end_date,
                            'karaoke_updated_at' => $query->karaoke_updated_at,
                        ),
                    );
                    $dataUpdate = array_merge($dataUpdate,$fields);
                }
                else {
                    $fields = array(
                        'action' => 'skip',
                    );
                    $dataUpdate = array_merge($dataUpdate, $fields);
                }
            }

        }
        return $dataUpdate;
    }

    public static function getNewSong($incomingData) {
        //1. Проверяем наличие минуса у песни
        $reserve = $incomingData;
        $incomingData = json_decode($incomingData, true);
        if ($incomingData[0]['hasMinus']) {
            $sessionID = PerepelKaraokeru::initConnection();
            $url = PerepelKaraokeru::$_url;
            $object = json_decode(PerepelKaraokeru::getSongData($incomingData[0]['id'], $url, $sessionID), true);
            $counter = count ($object) - 1;
            $dataUpdate = array();

            //2. Ищем есть ли песня в базе загруженных

            $query = db_select('karaokeru_songs_20','ks20')
                ->fields('ks20', array('nid','karaoke_id','end_date','karaoke_updated_at'))
                ->condition('karaoke_id', $incomingData[0]['id'], '=');
            $result = $query->execute()->fetchAll();

            //3. Если нет, то создаем блок данных для материала
            if (empty($result)) {

                //3.1 Записываем авторов в общую таблицу, если их там еще нет.
                $authorsPopulate = self::populateAuthors($reserve);
                $authors = array (
                    'artists' => $object[$counter]['Artists'],
                    'composers' => $object[$counter]['AuthorsMusic'],
                    'poets' => $object[$counter]['AuthorsText'],
                );
                $updateTime = strtotime($incomingData[0]['updated_at']);
                $dataUpdate = array(
                    'id' => $incomingData[0]['id'],
                    'name' => $incomingData[0]['title'],
                    'end_date' => strtotime($incomingData[0]['expire_at']),
                    'updated_at' => $updateTime,
                    'authors' => array($authors),
                    'mp3' => $object[$counter]['mp3'],
                    'dat' => $object[$counter]['dat'],
                    'license' => $incomingData[0]['copyrights'],
                    'lyrics' => $incomingData[0]['lyrics'],
                    'action' => 'add',
                );
                return ($dataUpdate);
            } else {
                $query = db_select('karaokeru_service', 'ks');
                $query->fields('ks', array('id'));
                $query->condition('ks.id', $incomingData[0]['id'], '=');
                $check2 = $query->execute()->fetchField();

                $fields = array (
                    'id' => $incomingData[0]['id'],
                    'updated_at' => strtotime($incomingData[0]['updated_at']),
                    'song_id' => $incomingData[0]['_self'],
                    'status' => 1,
                );

                if (empty($check2)) {

                    db_insert ('karaokeru_service')
                        ->fields($fields)
                        ->execute();
                }
                else {
                    db_update ('karaokeru_service')
                        ->fields($fields)
                        ->condition('id',$incomingData[0]['id'],'=')
                        ->execute();
                }
            }
        } else {
            $query = db_select('karaokeru_service', 'ks');
            $query->fields('ks', array('id'));
            $query->condition('ks.id', $incomingData[0]['id'], '=');
            $check2 = $query->execute()->fetchField();

            $fields = array (
                'id' => $incomingData[0]['id'],
                'updated_at' => strtotime($incomingData[0]['updated_at']),
                'song_id' => $incomingData[0]['_self'],
                'status' => 2,
            );

            if (empty($check2)) {

                db_insert ('karaokeru_service')
                    ->fields($fields)
                    ->execute();
            }
            else {
                db_update ('karaokeru_service')
                    ->fields($fields)
                    ->condition('id',$incomingData[0]['id'],'=')
                    ->execute();
            }

            return (2);
        }

        return $dataUpdate;
    }


    public static function getSongData($currentSong, $url, $sessionID) {
        $currentSong = json_encode($currentSong,true);
        $reqid = mt_rand();
        $data_song = array(
            "data" => array(
                "_reqid" => $reqid,
                "_request" => "Song_PlayRequest",
                "Session" => $sessionID,
                "Song" => $currentSong,
            ));
        $data = json_encode($data_song,true);

        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);

        $result = curl_exec($ch);

        curl_close($ch);  // Seems like good practice
        $json = json_decode($result, true);
        return $result;
    }

    public static function getSongDataNEW($currentSong, $sessionID) {
        $currentSong = json_encode($currentSong,true);
        $reqid = mt_rand();
        $url = '*****************_get?Song=' . $currentSong . '&Session=' . $sessionID . '&_recurse=Song.Artists,Song.Artists.Image,Song.Author';
        $data_song = array(
            "data" => array(
                "_reqid" => $reqid,
                "Session" => $sessionID,
                "Song" => $currentSong,
            ));
        $data = json_encode($data_song,true);

        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
        curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
        $result = curl_exec($ch);
        curl_close($ch);  // Seems like good practice
        $json = json_decode($result, true);
        return $result;
    }

    private static function ProcessAuthors($songData) {
        $authors = array ();
        foreach ($songData as $row) {
            if ($row["_class"] == 'SongModel') {
                $authorsSinger = $row['Artists'];
                $authorsText = $row['AuthorsText'];
                $authorsMusic = $row['AuthorsMusic'];
            }
        }
        foreach ($authorsSinger as $row) {
            foreach ($songData as $data) {
                if ($data['_class'] == 'AuthorModel') {
                    if ($data['_self'] == $row) {
                        $authors['artists'][] = $data['name'];
                        $authors['picture'] = $data['image_url'];
                    }
                } else continue;
            }
        }

        foreach ($authorsText as $row) {
            foreach ($songData as $data) {
                if ($data['_class'] == 'AuthorModel') {
                    if ($data['_self'] == $row) {
                        $authors['poets'][] = $data['name'];
                    }
                } else continue;
            }
        }

        foreach ($authorsMusic as $row) {
            foreach ($songData as $data) {
                if ($data['_class'] == 'AuthorModel') {
                    if ($data['_self'] == $row) {
                        $authors['composers'][] = $data['name'];
                    }
                } else continue;
            }
        }

        return $authors;
    }



    public static function updatingStarted() {
        return time() < variable_get(PKR_LOCK_EXPIRES, 0);
    }

    public static function getLock($leaseTime) {
        if (self::updatingStarted())
            return false;
        else {
            variable_set(PKR_LOCK_EXPIRES, time() + $leaseTime);
            return true;
        }
    }

    public static function releaseLock() {
        variable_set(PKR_LOCK_EXPIRES, 0);
    }

    public static function initConnection() {
        $reqid = mt_rand();
        $data = array(
            "data" => array(
                "_reqid" => $reqid,
                "_request" => "Session_GetRequest",
                "vendor_key" => self::$_vendorID,
                "country" => "ru",
                "lang" => "ru",
                "device_id" => "web-1",
                "version" => "2"
            ));
        $url_send ="*****************";
        $data = json_encode($data);
        try {
            $ch = curl_init($url_send);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_POST, true);
            curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
            curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
            $result = curl_exec($ch);
            curl_close($ch);  // Seems like good practice
            $json = json_decode($result, true);
            $session_id = $json[0]['Session'];
            return $session_id;
        } catch (Exception $e) {
            throw new PerepelException('Ошибка подключения к karaoke.ru', $e->getMessage());
        }
    }

    public static function processSongUpdate($song) {
        $updateResult = self::urIgnored;
        if ($song['action'] == 'update')
            $updateResult = self::updateSong($song) ? self::urUpdated : self::urSkipped;
        elseif ($song['action'] == 'add')
            $updateResult = self::addSong($song) ? self::urAdded : self::urIgnored;
        if (in_array($song['action'], array('update', 'skip'))) {
            db_update('karaokeru_songs_20')
                ->fields(array('sync_date' => REQUEST_TIME))
                ->condition('karaoke_id', $song['id'],'=')
                ->execute();
        }
        return $updateResult;
    }

    public static function importNewSong ($song) {
        try {
            //1.1 Get artist names from table
            $query = db_select ('artists_list', 'a')
                ->fields('a', array('name', 'fid'))
                ->condition('id', $song['authors'][0]['artists'], 'IN');
            $artists = $query->execute()->fetchAll();
            $performer = '';
            foreach ($artists as $row) {
                $performer = empty($performer) ? $row->name : $performer . ', ' . $row->name;
            }

            //1.2. Get music authors from table
            $query = db_select ('artists_list', 'a')
                ->fields('a', array('name', 'fid'))
                ->condition('id', $song['authors'][0]['composers'], 'IN');
            $composers = $query->execute()->fetchAll();
            $composer = '';
            foreach ($composers as $row) {
                $composer = empty($composer) ? $row->name : $composer . ', ' . $row->name;
            }

            //1.3. Get text authors from table
            $query = db_select ('artists_list', 'a')
                ->fields('a', array('name', 'fid'))
                ->condition('id', $song['authors'][0]['poets'], 'IN');
            $poets = $query->execute()->fetchAll();
            $poet = '';
            foreach ($poets as $row) {
                $poet = empty($poet) ? $row->name : $poet . ', ' . $row->name;
            }

            //2. Creating song name
            $songname = $performer . ' - ' . $song['name'];

            //3. Import mp3 & data files
            $minusFile = self::importMinusFile($song['id'], $song['mp3']);
            if ($minusFile) {
                if (!class_exists('MP3File')) {
                    include_once('/home/cvb/d1_perepel_ru/sites/all/modules/perepel_web_services/resources/mp3file.class.php');
                }
                $filetest = new MP3File(drupal_realpath($minusFile->uri));
                $duration = $filetest->getDuration();
                $minus = (array) $minusFile;
            }
            else {
                throw new PerepelException(sprintf('Не удалось загрузить файл минусовки для SID ' . $song['id'] . '(Karaoke.ru ID = %d). URL is ' . $song['mp3'], $song['id']), false, false);
            }
            $scenarioFile = self::importScenarioFile($song['id'], $song['dat']);
            if ($scenarioFile) {
                $scenario = (array) $scenarioFile;
            }
            else {
                throw new PerepelException(sprintf('Не удалось загрузить файл сценария для SID ' . $song['id'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
            }

            //4. Adding to the New category
            $newCategoryId = self::getNewCategoryId();
            if ($newCategoryId)
                $category = array('tid' => $newCategoryId);

            //5. Creating new node

            $language = LANGUAGE_NONE;
            $newsong = new stdClass();
            $newsong->type = 'paid';
            node_object_prepare($newsong);
            node_save($newsong);

            $newsong->title = $songname;
            $newsong->language = $language;
            $newsong->uid = 1;
            $newsong->field_minus[$language][0] = $minus;
            $newsong->field_scenario[$language][0] = $scenario;
            $newsong->field_artist[$language][0]['value'] = $performer;
            $newsong->field_songname[$language][0]['value'] = $song['name'];
            $newsong->field_poet[$language][0]['value'] = $poet;
            $newsong->field_composer[$language][0]['value'] = $composer;
            $newsong->field_license[$language][0]['value'] = $song['license'];
            $newsong->field_source[$language][0]['value'] = 1;
            $newsong->field_song_text[$language][0]['value'] = $song['lyrics'];
            $newsong->field_appleid[$language] = array(
                array('value' => 'ru.perepel.iphone.song_' . $newsong->nid),
                array('value' => 'ru.perepel.ipad.song_' . $newsong->nid),
                array('value' => 'ru.perepel.android.song_' . $newsong->nid),
            );
            $newsong->field_song_category[$language][] = $category;
            foreach ($artists as $row) {
                if (isset($row->fid)) {
                    $photo = $row->fid;
                    break;
                }
            }
            if (isset($photo)) {
                $newsong->field_photo_ipad[$language][0]['fid'] = $photo;
                $newsong->field_photo_ipad[$language][0]['display'] = 1;
            }
            node_save($newsong);

            //5.2 Adding music duration into DB

            $checkDB = db_select ('minus_time', 'mt')
                -> fields ('mt', array('entity_id'))
                -> condition ('entity_id', $newsong->nid);
            $checkResult = $checkDB->execute()->fetchField();

            if (empty ($checkResult)) {
                $queryFinal = db_insert('minus_time')->fields(array('entity_id', 'fid', 'time'));
                $queryFinal->values(array(
                        'entity_id' => $newsong->nid,
                        'fid' => $minusFile->fid,
                        'time' => $duration,
                    )
                );
                $queryFinal->execute();
            }

            //6. Saving service tables
            $fields = array(
                'nid' => $newsong->nid,
                'karaoke_id' => $song['id'],
                'end_date' => $song['end_date'],
                'karaoke_updated_at' => $song['updated_at'],
                'sync_date' => REQUEST_TIME,
            );

            $song_id = 'sng-' . $song['id'];
            $fields1 = array(
                'id' => $song['id'],
                'song_id' => $song_id,
                'updated_at' => $song['updated_at'],
            );

            $checkSongs = db_select ('karaokeru_songs_20', 'a')
                ->fields ('a', array ('nid'))
                ->condition ('karaoke_id', $song['id'], '=');
            $songExist = $checkSongs->execute()->fetchField();

            if (empty ($songExist)) {
                db_insert('karaokeru_songs_20')->fields($fields)->execute();
                $songID = $newsong->nid;
            } else {
                node_delete($newsong);
                $songID = (int) $songExist;
            }
            db_insert('karaokeru_service')->fields($fields1)->execute();

            $checksba = db_select ('songsbyartist', 'a')
                ->fields ('a', array('artist_id', 'nid'))
                ->condition('nid', $songID, '=');
            $sbaResult = $checksba->execute()->fetchAll();
            if (empty($sbaResult)) {
                foreach ($song['authors'][0]['artists'] as $row) {
                    $newartist = array (
                        'artist_id' => $row,
                        'nid' => $songID,
                        'type' => 'Artist',
                    );
                    db_insert ('songsbyartist')->fields($newartist)->execute();

                    foreach ($song['authors'][0]['composers'] as $row) {
                        $newcomposer = array (
                            'artist_id' => $row,
                            'nid' => $songID,
                            'type' => 'Music',
                        );
                        db_insert ('songsbyartist')->fields($newcomposer)->execute();
                    }
                }

                foreach ($song['authors'][0]['poets'] as $row) {
                    $newpoet = array (
                        'artist_id' => $row,
                        'nid' => $songID,
                        'type' => 'Text',
                    );
                    db_insert ('songsbyartist')->fields($newpoet)->execute();
                }
            }
        }
        catch (Exception $e) {
            watchdog('perepel_karaokeru', 'Error importing song ' . $song['id'] . ': ' . $e->getMessage(), array(), WATCHDOG_ERROR);
        }
    }


    public static function updateSong($song) {
        $result = false;
        $free = false;
        $node = node_load($song['perepel_data']['nid']);
        if ($node) {

            foreach (array('artists', 'poets', 'composers') as $authorType) {
                $names = array();
                if (is_array($song['authors']) && array_key_exists($authorType, $song['authors'][0]))
                    foreach ($song['authors'][0][$authorType] as $author) $names[] = $author;


                $song[$authorType] = implode(', ', $names);
            }
            $language = LANGUAGE_NONE;
            $title = ($song['artists'] ? $song['artists'] . ' - ' : '') . $song['name'];
            $changed = false;
            try {
                if ($node->title != $title && !isset($node->title)) {
                    $node->title = $title;
                    $changed = true;
                }
                if ($node->field_songname[$language][0]['value'] != $song['name'] && !isset($node->field_songname[$language][0]['value'])) {
                    $node->field_songname[$language][0]['value'] = $song['name'];
                    $changed = true;
                }
                if ($song['artists'] && (!isset($node->field_artist[$language][0]['value'])
                        || $node->field_artist[$language][0]['value'] != $song['artists'])) {
                    $node->field_artist[$language][0]['value'] = $song['artists'];
                    $changed = true;
                }
                if ($song['poets'] && (!isset($node->field_poet[$language][0]['value'])
                        || $node->field_poet[$language][0]['value'] != $song['poets'])) {
                    $node->field_poet[$language][0]['value'] = $song['poets'];
                    $changed = true;
                }
                if ($song['composers'] && (!isset($node->field_composer[$language][0]['value'])
                        || $node->field_composer[$language][0]['value'] != $song['composers'])) {
                    $node->field_composer[$language][0]['value'] = $song['composers'];
                    $changed = true;
                }

                if ($song['license']) $node->field_license[$language][0]['value'] = $song['license'];

                if (empty($node->field_minus[$language][0]['uri'])) {
                    $minusFile = self::importMinusFile($song['id'], $song['mp3']);
                    if ($minusFile) {
                        $node->field_minus[$language][0] = (array) $minusFile;
                        $changed = true;
                    }
                    else {
                        throw new PerepelException(sprintf('Не удалось загрузить файл минусовки для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
                        $changed = false;
                    }
                }
                if (empty($node->field_scenario[$language][0]['uri'])) {
                    $scenarioFile = self::importScenarioFile($song['id'], $song['dat']);
                    if ($scenarioFile) {
                        $node->field_scenario[$language][0] = (array) $scenarioFile;
                        $changed = true; }
                    else {
                        throw new PerepelException(sprintf('Не удалось загрузить файл сценария для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
                        $changed = false;
                    }
                }

                $node->field_source[$language][0]['value'] = 1;
                $newCategoryId = self::getNewCategoryId();
                if ($newCategoryId)
                    $node->field_song_category[$language][] = array('tid' => $newCategoryId);
                if ($free && $freeCategoryId = self::getFreeCategoryId())
                    $node->field_song_category[$language][] = array('tid' => $freeCategoryId);
                if (empty($node->field_photo_ipad[$language][0]['uri'])) {
                    if (isset($song['authors'][0]['picture'])) {
                        $file = self::importPreviewFile($song['authors'][0]['picture'], $song['id']);
                        if ($file) {
                            $node->field_photo_ipad[$language][0] = (array) $file;
                            $changed = true;
                        }
                    }
                }
                $node->changed = time();
                $node->status = 1;
            }
            catch (Exception $e) {}
            if ($changed) {
                node_save($node);
                $result = true;
            }
            $fields = array('karaoke_updated_at' => $song['updated_at']);
            if ($song['perepel_data']['end_date'] != $song['end_date']) $fields['end_date'] = $song['end_date'];

            db_update('karaokeru_songs_20')
                ->fields($fields)
                ->condition('karaoke_id', $song['id'])
                ->execute();
            if ($changed) $result = true;
        }
        return $result;
    }

    public static function addSong($song) {
        // If the song was already imported do nothing:
        $nid = db_select('karaokeru_songs_20', 'ks')
            ->fields('ks', array('nid'))
            ->condition('karaoke_id', $song['id'])
            ->range(0, 1)
            ->execute()->fetchField();
        if (is_numeric($nid)) {
            return true;
        }

        // Add a new node:
        $free = false;

        foreach (array('artists', 'poets', 'composers') as $authorType) {
            $names = array();
            if (is_array($song['authors']) && array_key_exists($authorType, $song['authors'][0]))
                foreach ($song['authors'][0][$authorType] as $author) $names[] = $author;

            $song[$authorType] = implode(', ', $names);

            if (!$free && strpos(mb_strtolower($song[$authorType]), 'народн') !== false)
                $free = true;
        };

        $nodeTitle = ($song['artists'] ? $song['artists'] . ' - ' : '') . $song['name'];
        $query = db_select('node', 'n')
            ->fields('n', array('nid', 'title'))
            ->condition('title', $nodeTitle, '=')
            ->condition('type', 'paid', '=')
            ->execute()
            ->fetchField();
        if (!empty($query)) {
            $language = LANGUAGE_NONE;
            $node = node_load($query->nid);
            if ($node->field_artist[$language][0]['value'] != $song['artists']) $node->field_artist[$language][0]['value'] = $song['artists'];
            if ($node->field_poet[$language][0]['value'] != $song['poets']) $node->field_poet[$language][0]['value'] = $song['poets'];
            if ($node->field_composer[$language][0]['value'] != $song['composers']) $node->field_composer[$language][0]['value'] = $song['composers'];
            if ($node->field_license[$language][0]['value'] != $song['license']) $node->field_license[$language][0]['value'] = $song['license'];

            if (empty($node->field_minus[$language][0]['uri'])) {
                $minusFile = self::importMinusFile($song['id'], $song['mp3']);
                if ($minusFile) {
                    $node->field_minus[$language][0] = (array) $minusFile;
                } else
                    throw new PerepelException(sprintf('Не удалось загрузить файл минусовки для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
            }

            if (empty($node->field_minus[$language][0]['uri'])) {
                $scenarioFile = self::importScenarioFile($song['id'], $song['dat']);
                if ($scenarioFile) {
                    $node->field_scenario[$language][0] = (array) $scenarioFile;
                }
                else
                    throw new PerepelException(sprintf('Не удалось загрузить файл сценария для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
            }

            $node->field_source[$language][0]['value'] = 1;
            $newCategoryId = self::getNewCategoryId();
            if ($newCategoryId)
                $node->field_song_category[$language][] = array('tid' => $newCategoryId);
            if ($free && $freeCategoryId = self::getFreeCategoryId())
                $node->field_song_category[$language][] = array('tid' => $freeCategoryId);
            if (isset($song['authors'][0]['picture']) && $file = self::importPreviewFile($song['authors'][0]['picture'], $song['id'])) {
                $node->field_photo_ipad[$language][0] = (array) $file;
            }
            $node->changed = time();

            if (!$free) {
                $node->field_appleid[$language] = array(
                    array('value' => 'ru.perepel.iphone.song_' . $node->nid),
                    array('value' => 'ru.perepel.ipad.song_' . $node->nid),
                    array('value' => 'ru.perepel.android.song_' . $node->nid),
                );
            }
            node_save($node);

            $fields = array(
                'nid' => $node->nid,
                'karaoke_id' => $song['id'],
                'end_date' => $song['end_date'],
                'karaoke_updated_at' => $song['updated_at'],
                'sync_date' => REQUEST_TIME,
            );

            $song_id = 'sng-' . $song['id'];
            $fields1 = array(
                'id' => $song['id'],
                'song_id' => $song_id,
                'updated_at' => $song['updated_at'],
            );

        } else {

            $language = LANGUAGE_NONE;
            $node = new stdClass();
            $node->type = 'paid';
            node_object_prepare($node);
            $node->title = ($song['artists'] ? $song['artists'] . ' - ' : '') . $song['name'];
            $node->language = $language;
            $node->uid = 1;

            $node->field_songname[$language][0]['value'] = $song['name'];
            if ($song['artists']) $node->field_artist[$language][0]['value'] = $song['artists'];
            if ($song['poets']) $node->field_poet[$language][0]['value'] = $song['poets'];
            if ($song['composers']) $node->field_composer[$language][0]['value'] = $song['composers'];
            if ($song['license']) $node->field_license[$language][0]['value'] = $song['license'];
            $minusFile = self::importMinusFile($song['id'], $song['mp3']);
            if ($minusFile) {
                $node->field_minus[$language][0] = (array) $minusFile;
            }
            else
                throw new PerepelException(sprintf('Не удалось загрузить файл минусовки для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);
            $scenarioFile = self::importScenarioFile($song['id'], $song['dat']);
            if ($scenarioFile) {
                $node->field_scenario[$language][0] = (array) $scenarioFile;
            }
            else
                throw new PerepelException(sprintf('Не удалось загрузить файл сценария для NID ' . $song['perepel_data']['nid'] . '(Karaoke.ru ID = %d). URL is ' . $song['dat'], $song['id']), false, false);

            $node->field_source[$language][0]['value'] = 1;
            $newCategoryId = self::getNewCategoryId();
            if ($newCategoryId)
                $node->field_song_category[$language][] = array('tid' => $newCategoryId);
            if ($free && $freeCategoryId = self::getFreeCategoryId())
                $node->field_song_category[$language][] = array('tid' => $freeCategoryId);
            if (isset($song['authors'][0]['picture']) && $file = self::importPreviewFile($song['authors'][0]['picture'], $song['id'])) {
                $node->field_photo_ipad[$language][0] = (array) $file;
            }
            if (empty($node->field_minus[$language][0]) || empty($node->field_scenario[$language][0])) $node->status = 0;

            if (!$free) {
                $node->field_appleid[$language] = array(
                    array('value' => 'ru.perepel.iphone.song_' . $node->nid),
                    array('value' => 'ru.perepel.ipad.song_' . $node->nid),
                    array('value' => 'ru.perepel.android.song_' . $node->nid),
                );
            }
            node_save($node);
            if (!is_numeric($node->nid))
                throw new PerepelException(sprintf('Не удалось сохранить ноду песни (Karaoke.ru ID = %d).', $song['id']), false, false);


            // Add a record into additional table:
            $fields = array(
                'nid' => $node->nid,
                'karaoke_id' => $song['id'],
                'end_date' => $song['end_date'],
                'karaoke_updated_at' => $song['updated_at'],
                'sync_date' => REQUEST_TIME,
            );

            $song_id = 'sng-' . $song['id'];
            $fields1 = array(
                'id' => $song['id'],
                'song_id' => $song_id,
                'updated_at' => $song['updated_at'],
            );
        }

        try {
            db_insert('karaokeru_songs_20')->fields($fields)->execute();
            db_insert('karaokeru_service')->fields($fields1)->execute();
        } catch (Exception $e) {
            node_delete($node->nid);
            throw $e;
        }
        return true;
    }

    private static function importMinusFile($id, $url) {
        if ($id) {
            $filename = "karaokeru-minus-$id.mp3";
            $file = self::getFileByName($filename);
            if ($file) return $file;
            try {
                $data = self::getUrlContent($url);
                if ($data) {
                    $dir = file_default_scheme() . '://catalogue/';
                    $file = file_save_data($data, $dir . $filename);
                    $file->display = 1;
                    return $file;
                }
            } catch (Exception $e) {}
        }
        return false;
    }

    public static function getFileByName($filename) {
        try {
            $fid = db_select('file_managed', 'f')
                ->fields('f', array('fid'))
                ->condition('filename', $filename)
                ->orderBy('fid', 'DESC')
                ->range(0, 1)
                ->execute()
                ->fetchField();
            if ($fid && $file = file_load($fid)) {
                $file->display = 1;
                return $file;
            }
        } catch (Exception $e) {}
        return false;
    }

    private static function importScenarioFile($urlkey, $url) {
        if ($urlkey) {
            $filename = "karaokeru-scenario-$urlkey.dat";
            $file = self::getFileByName($filename);
            if ($file) return $file;
            try {
                $data = self::getUrlContent($url);
                if ($data) {
                    $dir = file_default_scheme() . '://catalogue/';
                    $file = file_save_data($data, $dir . $filename);
                    $file->display = 1;
                    return $file;
                }
            } catch (Exception $e) {}
        }
        return false;
    }

    private static function getNewCategoryId() {
        if (is_null(self::$_newCategoryId)) {
            self::$_newCategoryId = false;
            $terms = taxonomy_get_term_by_name('Новые', 'song_categories');
            foreach ($terms as $index => $data) {
                self::$_newCategoryId = $index;
                break;
            }
        }
        return self::$_newCategoryId;
    }

    private static function getFreeCategoryId() {
        if (is_null(self::$_freeCategoryId)) {
            self::$_freeCategoryId = false;
            $terms = taxonomy_get_term_by_name('Бесплатные', 'song_categories');
            foreach ($terms as $index => $data) {
                self::$_freeCategoryId = $index;
                break;
            }
        }
        return self::$_freeCategoryId;
    }

    private static function importPreviewFile($url, $karaokeFileId) {
        if ($karaokeFileId) {
            $filename = basename($url);
            if ($file = self::getFileByName($filename)) return $file;
            try {
                $data = self::getUrlContent($url);
                if ($data && $image = self::getSquareImage(imagecreatefromstring($data))) {
                    ob_start();
                    imagejpeg($image);
                    $data = ob_get_clean();
                    $dir = file_default_scheme() . '://catalogue/';
                    $file = file_save_data($data, $dir . $filename);
                    $file->display = 1;
                    return $file;
                }
            } catch (Exception $e) {}
        }
        return false;
    }

    private static function getSquareImage($image) {
        if ($image) {
            $x = 0;
            $y = 0;
            $width = imagesx($image);
            $height = imagesy($image);
            if ($width > $height) {
                $x = intval(($width - $height)/2);
                $width = $height;
            } elseif ($height > $width) {
                $y = intval(($height - $width)/2);
                $height = $width;
            } else
                return $image;
            return imagecrop($image, array('x' => $x, 'y' => $y, 'width' => $width, 'height' => $height));
        }
        return false;
    }


    private static function getUrlContent($url) {
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, trim($url));
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 20);
        curl_setopt($ch, CURLOPT_TIMEOUT, 40);
        $data = curl_exec($ch);
        $httpcode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        return ($httpcode >= 200 && $httpcode < 300) ? $data : false;
    }


    public static function getContent($url) {
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, trim($url));
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, 20);
        curl_setopt($ch, CURLOPT_TIMEOUT, 40);
        $data = curl_exec($ch);
        $httpcode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        return ($httpcode >= 200 && $httpcode < 300) ? $data : false;
    }


    public static function getBatchUpdateLimit() { return self::$_batchUpdateLimit; }

    const urIgnored = 0;
    const urSkipped = 1;
    const urUpdated = 2;
    const urAdded = 3;

    private static $_conn = null;
    private static $_batchUpdateLimit = 10;
    private static $_pictureSizeW = 300;
    private static $_pictureSizeH = 200;
    private static $_newCategoryId = 45;
    private static $_freeCategoryId = 52;
    public static $_url = "******************";
    private static $_vendorID = "*******************";

    public static function getListUpdate($catalogue, $sessionID, $url, $limit) {
        $list = false;
        $i = 0;
        $catalogue = $catalogue[0]['Songs'];
        $sngArray = db_select('karaokeru_service','ksvc')
            ->fields('ksvc', array('song_id'))
            ->execute()->fetchAll();

        foreach ($sngArray as $row) {
            $songs2check[] = $row->song_id;
        }

        foreach ($catalogue as $row) {
            if (!array_search($row, $songs2check)) {
                $list[] = array(
                    'id' => $row,
                    'updated_at' => time(),
                );

                $list2[] = $row;
                $i++;
            }
            if ($i == $limit) break;
        }
        return $list;
    }

    public static function getListExpired () {
        $list = false;
        $i = 0;
        $songsExpired = db_select('karaokeru_songs_20','ks20')
            ->fields('ks20', array('karaoke_id'))
            ->condition('end_date', time(), '<')
            ->execute()->fetchAll();

        foreach ($songsExpired as $row) {
            $list[] = array(
                'id' => $row,
                'updated_at' => time(),
            );
        }
        return count($list);
    }

    public static function populateAuthors ($song) {
        $song = json_decode($song, TRUE);

        $artists = array();
        foreach ($song as $row) {
            $i = 0;
            if (isset($row["_class"])) {
                switch ($row["_class"]) {
                    case 'SongModel' :
                        $data['songId'] = $row['_self'];
                        $data['id'] = $row['id'];
                        $data['songLyrics'] = $row['lyrics'];
                        $data['expire'] = $row['expire_at'];
                        $data['hasMinus'] = $row['hasMinus'];
                        $data['Artists'] = $row['Artists'];
                        $data['Text'] = $row['AuthorsText'];
                        $data['Music'] = $row['AuthorsMusic'];
                        break;
                    case 'AuthorModel' :

                        if (!in_array($data['songId'], $row['Songs'], TRUE)) {
                            array_push($row['Songs'], $data['songId']);
                        }
                        $artist['artistId'] = $row['_self'];
                        $artist['name'] = $row['name'];
                        $artist['id'] = $row['id'];
                        if (!empty ($row['Songs'])) {
                            $listArtists = $row['Songs'];
                            $artist['Songs'] = array();
                            foreach ($listArtists as $record) {
                                array_push ($artist['Songs'], $record);
                            }
                        }
                        $artist['image'] = $row['image_url'];
                        if (in_array($row['_self'], $data['Artists'], TRUE)) {
                            $artist['type'] = 'Artist';
                            array_push ($artists, $artist);
                            $artistName = (empty($artistName) ? $artist['name'] : $artistName . ', ' . $artist['name']);
                        }
                        if (in_array($row['_self'], $data['Music'], TRUE)) {
                            $musician = array();
                            $musician['artistId'] = $row['_self'];
                            $musician['name'] = $row['name'];
                            $musician['id'] = $row['id'];
                            $musician['Songs'] = $row['Songs'];
                            $musician['image'] = $row['image_url'];
                            $musician['type'] = 'Music';
                            array_push ($artists, $musician);

                        }
                        if (in_array($row['_self'], $data['Text'], TRUE)) {
                            $writer = array();
                            $writer['artistId'] = $row['_self'];
                            $writer['name'] = $row['name'];
                            $writer['id'] = $row['id'];
                            $writer['Songs'] = $row['Songs'];
                            $writer['image'] = $row['image_url'];
                            $writer['type'] = 'Text';
                            array_push ($artists, $writer);
                        }
                        break;
                    default :
                        break;
                }
            }
        }
        foreach ($artists as $row) {

            //1. Adding to the list of artists

            $query = db_select ('artists_list', 'a');
            $query->fields('a',array('id', 'fid'));
            $query->condition('id', $row['artistId'], '=');
            $search = $query->execute()->fetchAll();

            if (empty ($search)) {
                $image = self:: importPreviewFile ($row['image'],$row['id']);
                if (empty ($image)) {
                    $fields = array(
                        'id' => $row['artistId'],
                        'name' => $row['name'],
                    );
                } else {
                    $fields = array(
                        'id' => $row['artistId'],
                        'name' => $row['name'],
                        'fid' => $image->fid,
                    );
                }
                db_insert ('artists_list')
                    ->fields($fields)
                    ->execute();
            } else {
                $image = array();
                if (($search[0]->fid == 0 || $search[0]->fid == 80914) && !empty($row['image'])) {
                    $image = self:: importPreviewFile ($row['image'],$row['id']);
                }
                $image_fid = (empty ($image->fid) ? 0 : $image->fid);
                db_update ('artists_list')
                    ->fields(array(
                        'name' => $row['name'],
                        'fid' => $image_fid,
                    ))
                    ->condition('id', $row['artistId'])
                    ->execute();
            }
            $songsList = $row['Songs'];

            //2.Adding list of songs by this artist

            foreach ($songsList as $song) {
                $query = db_select('karaokeru_songs_20', 's');
                $query->fields('s',array('nid'));
                $query->condition ('karaoke_id',(int) substr($song,4,10));
                $nid = $query->execute()->fetchField();

                if (!empty($nid)) {
                    $fields1 = array(
                        'artist_id' => $row['artistId'],
                        'nid' => $nid,
                        'type' =>$row['type'],
                    );
                    $check = db_select ('songsbyartist','s');
                    $check->fields('s', array('nid'));
                    $check->condition('artist_id',$row['artistId']);
                    $check->condition('nid',(int) $nid);
                    $check->condition('type', $row['type']);
                    $checkResult = $check->execute()->fetchAll();

                    if (empty($checkResult)) {
                        db_insert ('songsbyartist')
                            ->fields($fields1)
                            ->execute();

                    }

                    $newQuery = db_select ('field_data_field_artist','s');
                    $newQuery->fields('s', array('field_artist_value'));
                    $newQuery->condition('entity_id',(int) $nid);
                    $currentArtist = $newQuery->execute()->fetchField();

                    $currentArtist = (($currentArtist <> $artistName) ? $artistName : $currentArtist);

                    $artistUpdate = array (
                        'field_artist_value' => $currentArtist,
                    );

                    db_update('field_data_field_artist')
                        ->fields ($artistUpdate)
                        ->condition('entity_id', (int) $nid)
                        ->execute();

                    //3. Checking expire time
                    $query = db_select('karaokeru_songs_20', 's');
                    $query->fields('s',array('end_date'));
                    $query->condition ('karaoke_id',(int) substr($song,4,10));
                    $expire = $query->execute()->fetchField();
                    if ($expire < strtotime($data['expire'])) {
                        db_update ('karaokeru_songs_20')
                            ->fields(array(
                                'end_date' => strtotime($data['expire']),
                                'sync_date' => time(),
                            ))
                            ->condition('nid', $nid)
                            ->execute();
                    }
                }
            }



        }
        return TRUE;
    }


}
?>