<div class="msm-variables-help-item">
  <div class="main container-inline">
    <label><?php print $item['label']; ?>:</label>
    <span><?php print $item['value']; ?></span>
  </div>
  <?php if (!empty($item['description'])): ?>
  <div class="description"><?php print $item['description']; ?></div>
  <?php endif; ?>
  <?php if (!empty($item['documentation_url'])): ?>
  <?php $item += array('documentation_text' => t('Read the documentation »')); ?>
  <div class="docs"><?php print l($item['documentation_text'], $item['documentation_url']); ?></div>
  <?php endif; ?>
</div>
