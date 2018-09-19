<?php
/**
 * @file
 * Expose the migrate feature to hostmaster.
 */

/**
 * Implements hook_hosting_feature().
 */
function hosting_drushcommand_hosting_feature() {
  $features = array();

  $features['drushcommand'] = array(
    'title' => t('Site Drush Command'),
    'description' => t('Provides a Hosting task to run drush command on sites.'),
    'status' => HOSTING_FEATURE_ENABLED,
    'module' => 'hosting_drushcommand',
  );

  return $features;
}
