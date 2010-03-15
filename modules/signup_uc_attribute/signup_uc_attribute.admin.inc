<?php
// $Id$
/**
 * @file signup_uc_attribute.admin.inc
 */

function signup_uc_attribute_settings_form($form_state) {
  $form = array();
  
  $form['signup_uc_attribute_cart_timeout'] = array(
    '#type' => 'textfield',
    '#title' => t('Signup cart cancellation delay'),
    '#default_value' => variable_get('signup_uc_attribute_cart_timeout', 120),
    '#size' => 10,
    '#description' => t('The time (in minutes) that a signup will remain active while still in a cart. If the user has not completed their order in checkout after this time has elapsed, the signup will be cancelled. This prevents a signup-enabled product from filling up with unpaid signups.'),
  );  
  return system_settings_form($form);
}

