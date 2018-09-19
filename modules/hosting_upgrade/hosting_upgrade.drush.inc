<?php

/**
 * Implements hook_customdrushcommand().
 */
function hosting_upgrade_customdrushcommand() {
  return array(
    'upgrade' => array(
      'id' => 'upgrade',
      'title' => 'Upgrade site',
      'description' => 'Upgrade a site.',
      'examples' => array(
        'drush @site provision-upgrade "--security-only"' => 'Upgrade security only',
      ),
      'arguments' => array(
        'moreoption' => array(
          'title' => 'More options on upgrade',
          'default_value' => '--security-only',
          'description' => 'Ex: --security-only'
        ),
      ),
      'options' => array(),
      'map' => '[ -d .git ] && mv .git ../; drush up [vars:moreoption] --uri="[site:uri]" -y; [ -d ../.git ] && mv ../.git ./',
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    )
  );
}

/**
 * Drush Provision drushcommand task callback.
 *
 * Invoke upgrade via drush_hosting_drushcommand_execcustom
 */
function drush_hosting_upgrade_provision_upgrade() {
  $args = func_get_args();
  array_unshift($args, 'upgrade');
  return call_user_func_array('drush_hosting_drushcommand_execcustom', $args);
}

/**
 * Implements hook_hosting_tasks() of files
 */
function hosting_task_upgrade_form($node = array()) {
  return hosting_task_customcommand_form_exec('upgrade', $node);
}
