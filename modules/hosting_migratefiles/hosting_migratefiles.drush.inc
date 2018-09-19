<?php

/**
 * Implements hook_customdrushcommand().
 */
function hosting_migratefiles_customdrushcommand() {
  return array(
    'migratefiles' => array(
      'id' => 'migratefiles',
      'title' => 'Migrate files',
      'description' => 'Migrate files a site.',
      'examples' => array(
        'drush @site provision-migratefiles "sites" --exclude="default/files, cache"' => 'Migrate files from sites folder from local to remote server. Exclude files folder!',
      ),
      'arguments' => array(
        'relativedir' => array(
          'title' => 'Relative dir',
          'description' => 'Relative directory. Ex: sites/all, profiles, modules, themes'
        ),
      ),
      'options' => array('exclude' => array(
          'title' => 'Exclude options',
          'default_value' => 'sites/*/files, cache',
          'description' => 'Exclude directory. Ex: cache',
        )),
      'map' => 'rsync -ravzu [opts:exclude:hdc_options:--exclude] [vars:relativedir:hdc_addroot] [site:remote-user]@[site:remote-host]:[vars:relativedir:hdc_addroot:dirname]/',
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    )
  );
}

/**
 * Drush Provision drushcommand task callback.
 *
 * Invoke migrate files via drush_hosting_drushcommand_execcustom
 */
function drush_hosting_migratefiles_provision_migratefiles() {
  $args = func_get_args();
  array_unshift($args, 'migratefiles');
  return call_user_func_array('drush_hosting_drushcommand_execcustom', $args);
}

/**
 * Implements hook_hosting_tasks() of files
 */
function hosting_task_migratefiles_form($node = array()) {
  return hosting_task_customcommand_form_exec('migratefiles', $node);
}
